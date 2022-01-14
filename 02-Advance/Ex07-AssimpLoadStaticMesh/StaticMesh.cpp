#include "StaticMesh.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <filesystem>
#include "mesh_vert.inl"
#include "mesh_frag.inl"
#include "QImage"

StaticMesh::StaticMesh(QVulkanWindow* window, std::string file_path)
	: window_(window)
	, meshPath_(file_path) {
	scene = importer_.ReadFile(file_path, aiProcess_Triangulate | aiProcess_FlipUVs);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		printf("ERROR::ASSIMP:: %s", importer_.GetErrorString());
		return;
	}
	processMaterialTextures(scene);
	processNode(scene->mRootNode, scene, aiMatrix4x4());
}

void StaticMesh::processNode(const aiNode* node, const aiScene* scene, aiMatrix4x4 mat)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes_.push_back(std::make_shared<StaticMeshNode>(this, mesh, scene, mat));
	}
	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		processNode(node->mChildren[i], scene, mat * node->mChildren[i]->mTransformation);
	}
}

void StaticMesh::processMaterialTextures(const aiScene* scene)
{
	textures_.resize(scene->mNumMaterials);
	for (int i = 0; i < scene->mNumMaterials; i++) {
		aiMaterial* material = scene->mMaterials[i];
		for (auto type : textureTypes_) {
			for (int j = 0; j < material->GetTextureCount(type); j++) {
				aiString path;
				material->GetTexture(type, j, &path);
				auto item = textureSet_.find(path.C_Str());
				if (item != textureSet_.end()) {
					textures_[i].push_back(item->second);
				}
				else {
					auto texture = std::shared_ptr<StaticMeshNode::Texture>(new StaticMeshNode::Texture, [this](StaticMeshNode::Texture* texture) {
						if (texture->image)
							device_.destroyImage(texture->image);
						if (texture->imageView)
							device_.destroyImageView(texture->imageView);
						if (texture->imageMemory)
							device_.freeMemory(texture->imageMemory);
					});
					texture->path = path.C_Str();
					texture->type = type;
					textures_[i].push_back(texture);
					textureSet_[path.C_Str()] = texture;
				}
			}
		}
	}
}

void StaticMesh::initVulkanResource()
{
	Q_ASSERT(window_->device());
	device_ = window_->device();
	initVulkanTexture();
	initVulkanDescriptor();
	initVulkanPipline();
}

void StaticMesh::releaseVulkanResource()
{
	device_.destroyPipeline(pipline_);
	device_.destroyPipelineLayout(piplineLayout_);
	device_.destroyPipelineCache(piplineCache_);
	device_.destroyDescriptorPool(descPool_);
	device_.destroyDescriptorSetLayout(descSetLayout_);
	device_.destroySampler(commonSampler_);
	meshes_.clear();
	textures_.clear();
	textureSet_.clear();
}

void StaticMesh::makeRenderCommand(vk::CommandBuffer& cmdBuffer, QMatrix4x4 matrix)
{
	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipline_);
	for (auto& mesh : meshes_) {
		QMatrix4x4 localMatrix;
		memcpy(localMatrix.data(), &mesh->localMatrix_, sizeof(aiMatrix4x4));
		QMatrix4x4 flipY;
		flipY.scale(1, -1, 1);
		QMatrix4x4 mvp = matrix * flipY * localMatrix.transposed();
		cmdBuffer.pushConstants(piplineLayout_, vk::ShaderStageFlagBits::eVertex, 0, sizeof(float) * 16, mvp.constData());

		if (mesh->descSet_)
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, piplineLayout_, 0, 1, &mesh->descSet_, 0, nullptr);

		cmdBuffer.bindVertexBuffers(0, mesh->vertexBufferInfo_.buffer, mesh->vertexBufferInfo_.offset);
		cmdBuffer.bindIndexBuffer(mesh->indexBufferInfo_.buffer, mesh->indexBufferInfo_.offset, vk::IndexType::eUint32);
		cmdBuffer.drawIndexed(mesh->indexBufferInfo_.range / (sizeof(unsigned int)), 1, 0, 0, 0);
	}
}

void StaticMesh::initVulkanTexture() {
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eNearest;
	samplerInfo.minFilter = vk::Filter::eNearest;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.maxAnisotropy = 1.0f;
	commonSampler_ = device_.createSampler(samplerInfo);

	vk::CommandBufferAllocateInfo cmdBufferAllocInfo;
	cmdBufferAllocInfo.commandBufferCount = 1;
	cmdBufferAllocInfo.commandPool = window_->graphicsCommandPool();
	cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	vk::CommandBuffer cmdBuffer = device_.allocateCommandBuffers(cmdBufferAllocInfo).front();
	vk::CommandBufferBeginInfo cmdBufferBeginInfo;
	cmdBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	cmdBuffer.begin(cmdBufferBeginInfo);

	for (auto& textureIter : textureSet_) {
		std::shared_ptr<StaticMeshNode::Texture> texture = textureIter.second;
		texture->sampler = commonSampler_;
		QImage image(std::filesystem::path(meshPath_).parent_path().append(texture->path).string().c_str());
		if (image.isNull())
			continue;
		image = image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);

		vk::ImageCreateInfo imageInfo;
		imageInfo.imageType = vk::ImageType::e2D;
		imageInfo.format = vk::Format::eR8G8B8A8Unorm;
		imageInfo.extent.width = image.width();
		imageInfo.extent.height = image.height();
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = vk::SampleCountFlagBits::e1;
		imageInfo.tiling = vk::ImageTiling::eLinear;
		imageInfo.usage = vk::ImageUsageFlagBits::eSampled;
		imageInfo.initialLayout = vk::ImageLayout::eUndefined;
		texture->image = device_.createImage(imageInfo);

		vk::MemoryRequirements texMemReq = device_.getImageMemoryRequirements(texture->image);
		vk::MemoryAllocateInfo allocInfo(texMemReq.size, window_->hostVisibleMemoryIndex());
		texture->imageMemory = device_.allocateMemory(allocInfo);
		device_.bindImageMemory(texture->image, texture->imageMemory, 0);

		vk::ImageSubresource subres(vk::ImageAspectFlagBits::eColor, 0, 0/*imageInfo.mipLevels, imageInfo.arrayLayers*/);
		vk::SubresourceLayout subresLayout = device_.getImageSubresourceLayout(texture->image, subres);
		uint8_t* texMemPtr = (uint8_t*)device_.mapMemory(texture->imageMemory, subresLayout.offset, subresLayout.size);
		for (int y = 0; y < image.height(); ++y) {
			const uint8_t* imgLine = image.constScanLine(y);
			memcpy(texMemPtr + y * subresLayout.rowPitch, imgLine, image.width() * 4);
		}
		device_.unmapMemory(texture->imageMemory);

		vk::ImageViewCreateInfo imageViewInfo;
		imageViewInfo.image = texture->image;
		imageViewInfo.viewType = vk::ImageViewType::e2D;
		imageViewInfo.format = vk::Format::eR8G8B8A8Unorm;
		imageViewInfo.components.r = vk::ComponentSwizzle::eR;
		imageViewInfo.components.g = vk::ComponentSwizzle::eG;
		imageViewInfo.components.b = vk::ComponentSwizzle::eB;
		imageViewInfo.components.a = vk::ComponentSwizzle::eA;
		imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageViewInfo.subresourceRange.levelCount = imageViewInfo.subresourceRange.layerCount = 1;
		texture->imageView = device_.createImageView(imageViewInfo);

		vk::ImageMemoryBarrier barrier;
		barrier.image = texture->image;
		barrier.oldLayout = vk::ImageLayout::eUndefined;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		barrier.subresourceRange.layerCount = barrier.subresourceRange.levelCount = 1;
		cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);
	}

	cmdBuffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	vk::Queue queue = window_->graphicsQueue();
	queue.submit(submitInfo);
	queue.waitIdle();
}

void StaticMesh::initVulkanDescriptor()
{
	vk::DescriptorPoolSize descPoolSize(vk::DescriptorType::eCombinedImageSampler, (uint32_t)textureTypes_.size() * scene->mNumMeshes);
	vk::DescriptorPoolCreateInfo descPoolInfo;
	descPoolInfo.maxSets = scene->mNumMeshes;
	descPoolInfo.poolSizeCount = 1;
	descPoolInfo.pPoolSizes = &descPoolSize;
	descPool_ = device_.createDescriptorPool(descPoolInfo);

	std::vector<vk::DescriptorSetLayoutBinding>  layoutBinding(textureTypes_.size());
	for (int i = 0; i < textureTypes_.size(); i++) {
		layoutBinding[i] = { (uint32_t)textureTypes_[i], vk::DescriptorType::eCombinedImageSampler,1,vk::ShaderStageFlagBits::eFragment };
	}
	vk::DescriptorSetLayoutCreateInfo descLayoutInfo;
	descLayoutInfo.pNext = nullptr;
	descLayoutInfo.flags = {};
	descLayoutInfo.bindingCount = layoutBinding.size();
	descLayoutInfo.pBindings = layoutBinding.data();
	descSetLayout_ = device_.createDescriptorSetLayout(descLayoutInfo);

	for (auto& mesh : meshes_) {
		mesh->initVulkanResource(device_, window_->hostVisibleMemoryIndex());
	}
}

void StaticMesh::initVulkanPipline()
{
	vk::ShaderModuleCreateInfo shaderInfo;
	shaderInfo.codeSize = sizeof(mesh_vert);
	shaderInfo.pCode = mesh_vert;
	vk::ShaderModule vertShader = device_.createShaderModule(shaderInfo);

	shaderInfo.codeSize = sizeof(mesh_frag);
	shaderInfo.pCode = mesh_frag;
	vk::ShaderModule fragShader = device_.createShaderModule(shaderInfo);

	vk::GraphicsPipelineCreateInfo piplineInfo;
	piplineInfo.stageCount = 2;

	vk::PipelineShaderStageCreateInfo piplineShaderStage[2];
	piplineShaderStage[0].stage = vk::ShaderStageFlagBits::eVertex;
	piplineShaderStage[0].module = vertShader;
	piplineShaderStage[0].pName = "main";
	piplineShaderStage[1].stage = vk::ShaderStageFlagBits::eFragment;
	piplineShaderStage[1].module = fragShader;
	piplineShaderStage[1].pName = "main";
	piplineInfo.pStages = piplineShaderStage;
	piplineInfo.pStages = piplineShaderStage;

	vk::VertexInputBindingDescription vertexBindingDesc;
	vertexBindingDesc.binding = 0;
	vertexBindingDesc.stride = sizeof(StaticMeshNode::Vertex);
	vertexBindingDesc.inputRate = vk::VertexInputRate::eVertex;

	vk::VertexInputAttributeDescription vertexAttrDesc[5] = {
		{0,0,vk::Format::eR32G32B32Sfloat,offsetof(StaticMeshNode::Vertex,position)},
		{1,0,vk::Format::eR32G32B32Sfloat,offsetof(StaticMeshNode::Vertex,normal)},
		{2,0,vk::Format::eR32G32B32Sfloat,offsetof(StaticMeshNode::Vertex,tangent)},
		{3,0,vk::Format::eR32G32B32Sfloat,offsetof(StaticMeshNode::Vertex,bitangent)},
		{4,0,vk::Format::eR32G32Sfloat,offsetof(StaticMeshNode::Vertex,texCoords)},
	};

	vk::PipelineVertexInputStateCreateInfo vertexInputState({}, 1, &vertexBindingDesc, 5, vertexAttrDesc);
	piplineInfo.pVertexInputState = &vertexInputState;

	vk::PipelineInputAssemblyStateCreateInfo vertexAssemblyState({}, vk::PrimitiveTopology::eTriangleList);
	piplineInfo.pInputAssemblyState = &vertexAssemblyState;

	vk::PipelineViewportStateCreateInfo viewportState;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	piplineInfo.pViewportState = &viewportState;

	vk::PipelineRasterizationStateCreateInfo rasterizationState;
	rasterizationState.polygonMode = vk::PolygonMode::eFill;
	rasterizationState.cullMode = vk::CullModeFlagBits::eNone;
	rasterizationState.frontFace = vk::FrontFace::eCounterClockwise;
	rasterizationState.lineWidth = 1.0f;
	piplineInfo.pRasterizationState = &rasterizationState;

	vk::PipelineMultisampleStateCreateInfo MSState;
	MSState.rasterizationSamples = (vk::SampleCountFlagBits)window_->sampleCountFlagBits();
	piplineInfo.pMultisampleState = &MSState;

	vk::PipelineDepthStencilStateCreateInfo DSState;
	DSState.depthTestEnable = true;
	DSState.depthWriteEnable = true;
	DSState.depthCompareOp = vk::CompareOp::eLessOrEqual;
	piplineInfo.pDepthStencilState = &DSState;

	vk::PipelineColorBlendStateCreateInfo colorBlendState;
	colorBlendState.attachmentCount = 1;
	vk::PipelineColorBlendAttachmentState colorBlendAttachmentState;
	colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	colorBlendState.pAttachments = &colorBlendAttachmentState;
	piplineInfo.pColorBlendState = &colorBlendState;

	vk::PipelineDynamicStateCreateInfo dynamicState;
	vk::DynamicState dynamicEnables[] = { vk::DynamicState::eViewport ,vk::DynamicState::eScissor };
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicEnables;
	piplineInfo.pDynamicState = &dynamicState;

	vk::PipelineLayoutCreateInfo piplineLayoutInfo;

	vk::PushConstantRange pushConstantRange;
	pushConstantRange.size = sizeof(float) * 16;
	pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
	piplineLayoutInfo.pushConstantRangeCount = 1;
	piplineLayoutInfo.pPushConstantRanges = &pushConstantRange;
	piplineLayoutInfo.setLayoutCount = 1;
	piplineLayoutInfo.pSetLayouts = &descSetLayout_;
	piplineLayout_ = device_.createPipelineLayout(piplineLayoutInfo);

	piplineInfo.layout = piplineLayout_;

	piplineInfo.renderPass = window_->defaultRenderPass();

	piplineCache_ = device_.createPipelineCache(vk::PipelineCacheCreateInfo());
	pipline_ = device_.createGraphicsPipeline(piplineCache_, piplineInfo).value;

	device_.destroyShaderModule(vertShader);
	device_.destroyShaderModule(fragShader);
}