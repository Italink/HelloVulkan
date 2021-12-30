#include "StaticMeshRenderer.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include "mesh_vert.inl"
#include "mesh_frag.inl"

StaticMeshRenderer::StaticMeshRenderer(QVulkanWindow* window) : window_(window)
{
	camera_.setup(window);
}

void StaticMeshRenderer::loadFile(std::string file_path) {
	const aiScene* scene = importer_.ReadFile(file_path, aiProcess_Triangulate);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		printf("ERROR::ASSIMP:: %s", importer_.GetErrorString());
		return;
	}
	processNode(scene->mRootNode, scene, aiMatrix4x4());
}

void StaticMeshRenderer::initResources()
{
	loadFile("F:/QtVulkan/02-Advance/Ex05-Assimp/Genji/Genji.FBX");
	device = window_->device();
	vk::ShaderModuleCreateInfo shaderInfo;
	shaderInfo.codeSize = sizeof(mesh_vert);
	shaderInfo.pCode = mesh_vert;
	vk::ShaderModule vertShader = device.createShaderModule(shaderInfo);

	shaderInfo.codeSize = sizeof(mesh_frag);
	shaderInfo.pCode = mesh_frag;
	vk::ShaderModule fragShader = device.createShaderModule(shaderInfo);

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
	vertexBindingDesc.stride = sizeof(MeshNode::Vertex);
	vertexBindingDesc.inputRate = vk::VertexInputRate::eVertex;

	vk::VertexInputAttributeDescription vertexAttrDesc[5] = {
		{0,0,vk::Format::eR32G32B32Sfloat,offsetof(MeshNode::Vertex,position)},
		{1,0,vk::Format::eR32G32B32Sfloat,offsetof(MeshNode::Vertex,normal)},
		{2,0,vk::Format::eR32G32B32Sfloat,offsetof(MeshNode::Vertex,tangent)},
		{3,0,vk::Format::eR32G32B32Sfloat,offsetof(MeshNode::Vertex,bitangent)},
		{4,0,vk::Format::eR32G32Sfloat,offsetof(MeshNode::Vertex,texCoords)},
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

	piplineLayout_ = device.createPipelineLayout(piplineLayoutInfo);
	piplineInfo.layout = piplineLayout_;

	piplineInfo.renderPass = window_->defaultRenderPass();

	piplineCache_ = device.createPipelineCache(vk::PipelineCacheCreateInfo());
	pipline_ = device.createGraphicsPipeline(piplineCache_, piplineInfo).value;

	device.destroyShaderModule(vertShader);
	device.destroyShaderModule(fragShader);
}

void StaticMeshRenderer::initSwapChainResources()
{
}

void StaticMeshRenderer::releaseSwapChainResources()
{
}

void StaticMeshRenderer::releaseResources()
{
	device.destroyPipeline(pipline_);
	device.destroyPipelineLayout(piplineLayout_);
	device.destroyPipelineCache(piplineCache_);
	meshes_.clear();
}

void StaticMeshRenderer::startNextFrame()
{
	vk::CommandBuffer cmdBuffer = window_->currentCommandBuffer();
	const QSize size = window_->swapChainImageSize();

	vk::ClearValue clearValues[3] = {
		vk::ClearColorValue(std::array<float,4>{0.0f,0.0f,0.0f,1.0f }),
		vk::ClearDepthStencilValue(1.0f,0),
		vk::ClearColorValue(std::array<float,4>{ 0.0f,0.5f,0.9f,1.0f }),
	};

	vk::RenderPassBeginInfo beginInfo;
	beginInfo.renderPass = window_->defaultRenderPass();
	beginInfo.framebuffer = window_->currentFramebuffer();
	beginInfo.renderArea.extent.width = size.width();
	beginInfo.renderArea.extent.height = size.height();
	beginInfo.clearValueCount = window_->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
	beginInfo.pClearValues = clearValues;

	cmdBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

	vk::Viewport viewport;
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = size.width();
	viewport.height = size.height();

	viewport.minDepth = 0;
	viewport.maxDepth = 1;
	cmdBuffer.setViewport(0, viewport);

	vk::Rect2D scissor;
	scissor.offset.x = scissor.offset.y = 0;
	scissor.extent.width = size.width();
	scissor.extent.height = size.height();
	cmdBuffer.setScissor(0, scissor);

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipline_);

	for (auto& mesh : meshes_) {
		QMatrix4x4 localMatrix;
		memcpy(localMatrix.data(), &mesh->localMatrix_, sizeof(aiMatrix4x4));

		QMatrix4x4 flipY;
		flipY.scale(1, -1, 1);

		QMatrix4x4 mvp = camera_.getMatrix() * flipY * localMatrix.transposed();

		cmdBuffer.pushConstants(piplineLayout_, vk::ShaderStageFlagBits::eVertex, 0, sizeof(float) * 16, mvp.constData());
		cmdBuffer.bindVertexBuffers(0, mesh->vertexBufferInfo_.buffer, mesh->vertexBufferInfo_.offset);
		cmdBuffer.bindIndexBuffer(mesh->indexBufferInfo_.buffer, mesh->indexBufferInfo_.offset, vk::IndexType::eUint32);
		cmdBuffer.drawIndexed(mesh->indexBufferInfo_.range / (sizeof(unsigned int)), 1, 0, 0, 0);
	}

	cmdBuffer.endRenderPass();

	window_->frameReady();
	window_->requestUpdate();
}

void StaticMeshRenderer::processNode(const aiNode* node, const aiScene* scene, aiMatrix4x4 mat)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes_.push_back(std::make_shared<MeshNode>(window_->device(), this, mesh, scene, window_->hostVisibleMemoryIndex(), mat));
	}
	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		processNode(node->mChildren[i], scene, mat * node->mChildren[i]->mTransformation);
	}
}