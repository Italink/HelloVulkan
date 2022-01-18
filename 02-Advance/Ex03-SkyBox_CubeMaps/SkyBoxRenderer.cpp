#include "SkyBoxRenderer.h"
#include <QTime>
#include "skybox_vert.inl"
#include "skybox_frag.inl"

static float vertexData[] = { // Y up, front = CCW
		// positions
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
};

void SkyBoxRenderer::initResources()
{
	vk::Device device = window_->device();

	vk::BufferCreateInfo vertexBufferInfo;
	vertexBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
	vertexBufferInfo.size = sizeof(vertexData);
	vertexBuffer_ = device.createBuffer(vertexBufferInfo);
	vk::MemoryRequirements vertexMemReq = device.getBufferMemoryRequirements(vertexBuffer_);
	vk::MemoryAllocateInfo vertexMemAllocInfo(vertexMemReq.size, window_->hostVisibleMemoryIndex());
	vertexDevMemory_ = device.allocateMemory(vertexMemAllocInfo);
	device.bindBufferMemory(vertexBuffer_, vertexDevMemory_, 0);
	uint8_t* vertexBufferMemPtr = (uint8_t*)device.mapMemory(vertexDevMemory_, 0, vertexMemReq.size);
	memcpy(vertexBufferMemPtr, vertexData, sizeof(vertexData));
	device.unmapMemory(vertexDevMemory_);

	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eNearest;
	samplerInfo.minFilter = vk::Filter::eNearest;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.maxAnisotropy = 1.0f;
	sampler_ = device.createSampler(samplerInfo);

	QImage images[6] = {
		QImage(":/image/skybox/right.jpg").convertedTo(QImage::Format_RGBA8888_Premultiplied),
		QImage(":/image/skybox/left.jpg").convertedTo(QImage::Format_RGBA8888_Premultiplied),
		QImage(":/image/skybox/top.jpg").convertedTo(QImage::Format_RGBA8888_Premultiplied),
		QImage(":/image/skybox/bottom.jpg").convertedTo(QImage::Format_RGBA8888_Premultiplied),
		QImage(":/image/skybox/front.jpg").convertedTo(QImage::Format_RGBA8888_Premultiplied),
		QImage(":/image/skybox/back.jpg").convertedTo(QImage::Format_RGBA8888_Premultiplied),
	};

	vk::BufferCreateInfo stagingBufferInfo;
	stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	stagingBufferInfo.size = images[0].sizeInBytes() * 6;
	vk::Buffer stagingBuffer = device.createBuffer(stagingBufferInfo);
	vk::MemoryRequirements stagingMemReq = device.getBufferMemoryRequirements(stagingBuffer);
	vk::MemoryAllocateInfo stagingMemInfo(stagingMemReq.size, window_->hostVisibleMemoryIndex());
	vk::DeviceMemory stagingMemory = device.allocateMemory(stagingMemInfo);
	device.bindBufferMemory(stagingBuffer, stagingMemory, 0);

	uint8_t* stagingBufferMemPtr = (uint8_t*)device.mapMemory(stagingMemory, 0, stagingMemReq.size);
	for (int i = 0; i < 6; i++) {
		memcpy(stagingBufferMemPtr + i * images[i].sizeInBytes(), images[i].bits(), images[i].sizeInBytes());
	}
	device.unmapMemory(stagingMemory);

	vk::ImageCreateInfo imageInfo;
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.format = vk::Format::eR8G8B8A8Unorm;
	imageInfo.extent.width = images[0].width();
	imageInfo.extent.height = images[0].height();
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	imageInfo.tiling = vk::ImageTiling::eOptimal;
	imageInfo.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;

	imageInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
	imageInfo.arrayLayers = 6;

	image_ = device.createImage(imageInfo);
	vk::MemoryRequirements texMemReq = device.getImageMemoryRequirements(image_);
	vk::MemoryAllocateInfo allocInfo(texMemReq.size, window_->deviceLocalMemoryIndex());
	imageDevMemory_ = device.allocateMemory(allocInfo);
	device.bindImageMemory(image_, imageDevMemory_, 0);

	vk::ImageViewCreateInfo imageViewInfo;
	imageViewInfo.image = image_;
	imageViewInfo.viewType = vk::ImageViewType::eCube;
	imageViewInfo.format = vk::Format::eR8G8B8A8Unorm;
	imageViewInfo.components.r = vk::ComponentSwizzle::eR;
	imageViewInfo.components.g = vk::ComponentSwizzle::eG;
	imageViewInfo.components.b = vk::ComponentSwizzle::eB;
	imageViewInfo.components.a = vk::ComponentSwizzle::eA;
	imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	imageViewInfo.subresourceRange.levelCount = 1;
	imageViewInfo.subresourceRange.layerCount = 6;
	imageView_ = device.createImageView(imageViewInfo);

	vk::CommandBufferAllocateInfo cmdBufferAllocInfo;
	cmdBufferAllocInfo.commandBufferCount = 1;
	cmdBufferAllocInfo.commandPool = window_->graphicsCommandPool();
	cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	vk::CommandBuffer cmdBuffer = device.allocateCommandBuffers(cmdBufferAllocInfo).front();
	vk::CommandBufferBeginInfo cmdBufferBeginInfo;
	cmdBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	cmdBuffer.begin(cmdBufferBeginInfo);

	vk::ImageMemoryBarrier barrier;
	barrier.image = image_;
	barrier.oldLayout = vk::ImageLayout::eUndefined;
	barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
	barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.layerCount = 6;
	barrier.subresourceRange.levelCount = 1;
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, barrier);

	vk::BufferImageCopy bufferCopyRegion;
	bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	bufferCopyRegion.imageSubresource.mipLevel = 0;
	bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
	bufferCopyRegion.imageSubresource.layerCount = 6;
	bufferCopyRegion.imageExtent.width = images[0].width();
	bufferCopyRegion.imageExtent.height = images[0].height();
	bufferCopyRegion.imageExtent.depth = 1;
	bufferCopyRegion.bufferOffset = 0;

	cmdBuffer.copyBufferToImage(stagingBuffer, image_, vk::ImageLayout::eTransferDstOptimal, bufferCopyRegion);

	barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, barrier);

	cmdBuffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	vk::Queue queue = window_->graphicsQueue();
	queue.submit(submitInfo);
	queue.waitIdle();

	device.destroyBuffer(stagingBuffer);
	device.freeMemory(stagingMemory);

	vk::DescriptorPoolSize descPoolSize = {
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler,1)
	};

	vk::DescriptorPoolCreateInfo descPoolInfo;
	descPoolInfo.maxSets = 1;
	descPoolInfo.poolSizeCount = 1;
	descPoolInfo.pPoolSizes = &descPoolSize;
	descPool_ = device.createDescriptorPool(descPoolInfo);

	vk::DescriptorSetLayoutBinding layoutBinding[2] = {
		{0, vk::DescriptorType::eCombinedImageSampler,1,vk::ShaderStageFlagBits::eFragment}
	};

	vk::DescriptorSetLayoutCreateInfo descLayoutInfo;
	descLayoutInfo.pNext = nullptr;
	descLayoutInfo.flags = {};
	descLayoutInfo.bindingCount = 1;
	descLayoutInfo.pBindings = layoutBinding;

	descSetLayout_ = device.createDescriptorSetLayout(descLayoutInfo);

	vk::DescriptorSetAllocateInfo descSetAllocInfo(descPool_, 1, &descSetLayout_);
	descSet_ = device.allocateDescriptorSets(descSetAllocInfo).front();
	vk::WriteDescriptorSet descWrite;
	vk::DescriptorImageInfo descImageInfo(sampler_, imageView_, vk::ImageLayout::eShaderReadOnlyOptimal);
	descWrite.dstSet = descSet_;
	descWrite.dstBinding = 0;
	descWrite.descriptorCount = 1;
	descWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	descWrite.pImageInfo = &descImageInfo;
	device.updateDescriptorSets(1, &descWrite, 0, nullptr);

	vk::GraphicsPipelineCreateInfo piplineInfo;
	piplineInfo.stageCount = 2;

	vk::ShaderModuleCreateInfo shaderInfo;
	shaderInfo.codeSize = sizeof(skybox_vert);
	shaderInfo.pCode = skybox_vert;
	vk::ShaderModule vertShader = device.createShaderModule(shaderInfo);

	shaderInfo.codeSize = sizeof(skybox_frag);
	shaderInfo.pCode = skybox_frag;
	vk::ShaderModule fragShader = device.createShaderModule(shaderInfo);

	vk::PipelineShaderStageCreateInfo piplineShaderStage[2];
	piplineShaderStage[0].stage = vk::ShaderStageFlagBits::eVertex;
	piplineShaderStage[0].module = vertShader;
	piplineShaderStage[0].pName = "main";
	piplineShaderStage[1].stage = vk::ShaderStageFlagBits::eFragment;
	piplineShaderStage[1].module = fragShader;
	piplineShaderStage[1].pName = "main";
	piplineInfo.pStages = piplineShaderStage;

	vk::VertexInputBindingDescription vertexBindingDesc;
	vertexBindingDesc.binding = 0;
	vertexBindingDesc.stride = 3 * sizeof(float);
	vertexBindingDesc.inputRate = vk::VertexInputRate::eVertex;

	vk::VertexInputAttributeDescription vertexAttrDesc;
	vertexAttrDesc.binding = 0;
	vertexAttrDesc.location = 0;
	vertexAttrDesc.format = vk::Format::eR32G32B32Sfloat;
	vertexAttrDesc.offset = 0;

	vk::PipelineVertexInputStateCreateInfo vertexInputState({}, 1, &vertexBindingDesc, 1, &vertexAttrDesc);
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
	colorBlendAttachmentState.blendEnable = true;
	colorBlendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	colorBlendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	colorBlendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	colorBlendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	colorBlendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	colorBlendState.pAttachments = &colorBlendAttachmentState;
	piplineInfo.pColorBlendState = &colorBlendState;
	vk::PipelineDynamicStateCreateInfo dynamicState;
	vk::DynamicState dynamicEnables[] = { vk::DynamicState::eViewport ,vk::DynamicState::eScissor };
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicEnables;
	piplineInfo.pDynamicState = &dynamicState;

	vk::PipelineLayoutCreateInfo piplineLayoutInfo;
	piplineLayoutInfo.setLayoutCount = 1;
	piplineLayoutInfo.pSetLayouts = &descSetLayout_;

	vk::PushConstantRange pcRange;
	pcRange.size = sizeof(float) * 16;
	pcRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
	piplineLayoutInfo.pushConstantRangeCount = 1;
	piplineLayoutInfo.pPushConstantRanges = &pcRange;

	piplineLayout_ = device.createPipelineLayout(piplineLayoutInfo);
	piplineInfo.layout = piplineLayout_;

	piplineInfo.renderPass = window_->defaultRenderPass();

	piplineCache_ = device.createPipelineCache(vk::PipelineCacheCreateInfo());
	pipline_ = device.createGraphicsPipeline(piplineCache_, piplineInfo).value;

	device.destroyShaderModule(vertShader);
	device.destroyShaderModule(fragShader);
}

void SkyBoxRenderer::initSwapChainResources()
{
}

void SkyBoxRenderer::releaseSwapChainResources()
{
}

void SkyBoxRenderer::releaseResources() {
	vk::Device device = window_->device();
	device.destroyPipeline(pipline_);
	device.destroyPipelineCache(piplineCache_);
	device.destroyPipelineLayout(piplineLayout_);
	device.destroyDescriptorSetLayout(descSetLayout_);
	device.destroyDescriptorPool(descPool_);
	device.destroyBuffer(vertexBuffer_);
	device.freeMemory(vertexDevMemory_);
	device.destroySampler(sampler_);
	device.destroyImage(image_);
	device.freeMemory(imageDevMemory_);
	device.destroyImageView(imageView_);
}

void SkyBoxRenderer::startNextFrame() {
	vk::Device device = window_->device();
	vk::CommandBuffer cmdBuffer = window_->currentCommandBuffer();
	const QSize size = window_->swapChainImageSize();

	vk::ClearValue clearValues[3] = {
		vk::ClearColorValue(std::array<float,4>{1.0f,0.0f,0.0f,1.0f }),
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

	QMatrix4x4 matrix;
	matrix *= window_->camera_.getMatrix();
	matrix *= window_->clipCorrectionMatrix();
	cmdBuffer.pushConstants(piplineLayout_, vk::ShaderStageFlagBits::eVertex, 0, sizeof(float) * 16, matrix.constData());

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

	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, piplineLayout_, 0, 1, &descSet_, 0, nullptr);

	cmdBuffer.bindVertexBuffers(0, vertexBuffer_, { 0 });

	cmdBuffer.draw(36, 1, 0, 0);

	cmdBuffer.endRenderPass();
}