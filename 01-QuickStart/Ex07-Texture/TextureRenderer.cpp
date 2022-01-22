#include "TextureRenderer.h"
#include <QTime>
#include "Shaders/texture_vert.inl"
#include "Shaders/texture_frag.inl"

static float vertexData[] = { // Y up, front = CCW
	//position		tex_coord
	-0.5f, -0.5f,   0, 1,
	 0.5f, -0.5f,   1, 1,
	 0.5f,  0.5f,   1, 0,
	-0.5f,  0.5f,   0, 0,
};

static uint16_t indexData[] = {
	0,1,2,
	2,3,0
};

static inline vk::DeviceSize aligned(vk::DeviceSize v, vk::DeviceSize byteAlign) {
	return (v + byteAlign - 1) & ~(byteAlign - 1);
}

TextureRenderer::TextureRenderer(QVulkanWindow* window)
	:window_(window)
{
	QList<int> sampleCounts = window->supportedSampleCounts();
	if (!sampleCounts.isEmpty()) {
		window->setSampleCount(sampleCounts.back());
	}
}

void TextureRenderer::initResources()
{
	vk::Device device = window_->device();

	const int concurrentFrameCount = window_->concurrentFrameCount();
	vk::PhysicalDeviceLimits limits = window_->physicalDeviceProperties()->limits;

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

	vk::BufferCreateInfo indexBufferInfo;
	indexBufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;
	indexBufferInfo.size = sizeof(indexData);
	indexBuffer_ = device.createBuffer(indexBufferInfo);
	vk::MemoryRequirements indexMemReq = device.getBufferMemoryRequirements(indexBuffer_);
	vk::MemoryAllocateInfo indexMemInfo(indexMemReq.size, window_->hostVisibleMemoryIndex());
	indexDevMemory_ = device.allocateMemory(indexMemInfo);
	device.bindBufferMemory(indexBuffer_, indexDevMemory_, 0);
	uint8_t* indexBufferMemPtr = (uint8_t*)device.mapMemory(indexDevMemory_, 0, indexMemReq.size);
	memcpy(indexBufferMemPtr, indexData, sizeof(indexData));
	device.unmapMemory(indexDevMemory_);

	vk::DeviceSize uniformAllocSize = aligned(16 * sizeof(float), limits.minUniformBufferOffsetAlignment);
	vk::BufferCreateInfo uniformBufferInfo;
	uniformBufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	uniformBufferInfo.size = uniformAllocSize * concurrentFrameCount;
	uniformBuffer_ = device.createBuffer(uniformBufferInfo);
	vk::MemoryRequirements uniformMemReq = device.getBufferMemoryRequirements(uniformBuffer_);
	vk::MemoryAllocateInfo uniformMemAllocInfo(uniformMemReq.size, window_->hostVisibleMemoryIndex());
	uniformDevMemory_ = device.allocateMemory(uniformMemAllocInfo);
	device.bindBufferMemory(uniformBuffer_, uniformDevMemory_, 0);
	uint8_t* uniformBufferMemPtr = (uint8_t*)device.mapMemory(uniformDevMemory_, 0, uniformMemReq.size);
	QMatrix4x4 identify;
	for (int i = 0; i < concurrentFrameCount; i++) {
		vk::DeviceSize offset = i * uniformAllocSize;
		memcpy(uniformBufferMemPtr + offset, identify.constData(), 16 * sizeof(float));
		uniformBufferInfo_[i].buffer = uniformBuffer_;
		uniformBufferInfo_[i].offset = offset;
		uniformBufferInfo_[i].range = uniformAllocSize;
	}
	device.unmapMemory(uniformDevMemory_);

	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eNearest;
	samplerInfo.minFilter = vk::Filter::eNearest;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.maxAnisotropy = 1.0f;
	sampler_ = device.createSampler(samplerInfo);

	vk::PhysicalDevice physicalDevice = window_->physicalDevice();

	QImage img(":image/qt256.png");
	if (img.isNull()) {
		qWarning("Image is null");
		return;
	}
	img = img.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
	vk::FormatProperties formatProps = physicalDevice.getFormatProperties(vk::Format::eR8G8B8A8Unorm);
	auto canSampleLinear = (formatProps.linearTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage);
	auto canSampleOptimal = (formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage);
	if (!canSampleLinear && !canSampleOptimal) {
		qWarning("Neither linear nor optimal image sampling is supported for RGBA8");
	}
	vk::ImageCreateInfo imageInfo;
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.format = vk::Format::eR8G8B8A8Unorm;
	imageInfo.extent.width = img.width();
	imageInfo.extent.height = img.height();
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	imageInfo.tiling = vk::ImageTiling::eLinear;
	imageInfo.usage = vk::ImageUsageFlagBits::eSampled;
	imageInfo.initialLayout = vk::ImageLayout::ePreinitialized;
	image_ = device.createImage(imageInfo);

	vk::MemoryRequirements texMemReq = device.getImageMemoryRequirements(image_);
	vk::MemoryAllocateInfo allocInfo(texMemReq.size, window_->hostVisibleMemoryIndex());
	imageDevMemory_ = device.allocateMemory(allocInfo);
	device.bindImageMemory(image_, imageDevMemory_, 0);

	vk::ImageSubresource subres(vk::ImageAspectFlagBits::eColor, 0, 0/*imageInfo.mipLevels, imageInfo.arrayLayers*/);
	vk::SubresourceLayout subresLayout = device.getImageSubresourceLayout(image_, subres);
	uint8_t* texMemPtr = (uint8_t*)device.mapMemory(imageDevMemory_, subresLayout.offset, subresLayout.size);
	for (int y = 0; y < img.height(); ++y) {
		const uint8_t* imgLine = img.constScanLine(y);
		memcpy(texMemPtr + y * subresLayout.rowPitch, imgLine, img.width() * 4);
	}
	device.unmapMemory(imageDevMemory_);

	vk::ImageViewCreateInfo imageViewInfo;
	imageViewInfo.image = image_;
	imageViewInfo.viewType = vk::ImageViewType::e2D;
	imageViewInfo.format = vk::Format::eR8G8B8A8Unorm;
	imageViewInfo.components.r = vk::ComponentSwizzle::eR;
	imageViewInfo.components.g = vk::ComponentSwizzle::eG;
	imageViewInfo.components.b = vk::ComponentSwizzle::eB;
	imageViewInfo.components.a = vk::ComponentSwizzle::eA;
	imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	imageViewInfo.subresourceRange.levelCount = imageViewInfo.subresourceRange.layerCount = 1;
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
	barrier.oldLayout = vk::ImageLayout::ePreinitialized;
	barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.layerCount = barrier.subresourceRange.levelCount = 1;
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);
	cmdBuffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	vk::Queue queue = window_->graphicsQueue();
	queue.submit(submitInfo);
	queue.waitIdle();

	vk::DescriptorPoolSize descPoolSize[2] = {
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, (uint32_t)concurrentFrameCount),
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, (uint32_t)concurrentFrameCount)
	};

	vk::DescriptorPoolCreateInfo descPoolInfo;
	descPoolInfo.maxSets = concurrentFrameCount;
	descPoolInfo.poolSizeCount = 2;
	descPoolInfo.pPoolSizes = descPoolSize;
	descPool_ = device.createDescriptorPool(descPoolInfo);

	vk::DescriptorSetLayoutBinding layoutBinding[2] = {
		{0, vk::DescriptorType::eUniformBuffer,1,vk::ShaderStageFlagBits::eVertex},
		{1, vk::DescriptorType::eCombinedImageSampler,1,vk::ShaderStageFlagBits::eFragment}
	};

	vk::DescriptorSetLayoutCreateInfo descLayoutInfo;
	descLayoutInfo.pNext = nullptr;
	descLayoutInfo.flags = {};
	descLayoutInfo.bindingCount = 2;
	descLayoutInfo.pBindings = layoutBinding;

	descSetLayout_ = device.createDescriptorSetLayout(descLayoutInfo);

	for (int i = 0; i < concurrentFrameCount; ++i) {
		vk::DescriptorSetAllocateInfo descSetAllocInfo(descPool_, 1, &descSetLayout_);
		descSet_[i] = device.allocateDescriptorSets(descSetAllocInfo).front();
		vk::WriteDescriptorSet descWrite[2];
		descWrite[0].dstSet = descSet_[i];
		descWrite[0].dstBinding = 0;
		descWrite[0].descriptorCount = 1;
		descWrite[0].descriptorType = vk::DescriptorType::eUniformBuffer;
		descWrite[0].pBufferInfo = &uniformBufferInfo_[i];

		vk::DescriptorImageInfo descImageInfo(sampler_, imageView_, vk::ImageLayout::eShaderReadOnlyOptimal);
		descWrite[1].dstSet = descSet_[i];
		descWrite[1].dstBinding = 1;
		descWrite[1].descriptorCount = 1;
		descWrite[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descWrite[1].pImageInfo = &descImageInfo;
		device.updateDescriptorSets(2, descWrite, 0, nullptr);
	}

	vk::GraphicsPipelineCreateInfo piplineInfo;
	piplineInfo.stageCount = 2;

	vk::ShaderModuleCreateInfo shaderInfo;
	shaderInfo.codeSize = sizeof(texture_vert);
	shaderInfo.pCode = texture_vert;
	vk::ShaderModule vertShader = device.createShaderModule(shaderInfo);

	shaderInfo.codeSize = sizeof(texture_frag);
	shaderInfo.pCode = texture_frag;
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
	vertexBindingDesc.stride = 4 * sizeof(float);
	vertexBindingDesc.inputRate = vk::VertexInputRate::eVertex;

	vk::VertexInputAttributeDescription vertexAttrDesc[2];
	vertexAttrDesc[0].binding = 0;
	vertexAttrDesc[0].location = 0;
	vertexAttrDesc[0].format = vk::Format::eR32G32Sfloat;
	vertexAttrDesc[0].offset = 0;
	vertexAttrDesc[1].binding = 0;
	vertexAttrDesc[1].location = 1;
	vertexAttrDesc[1].format = vk::Format::eR32G32Sfloat;
	vertexAttrDesc[1].offset = 2 * sizeof(float);

	vk::PipelineVertexInputStateCreateInfo vertexInputState({}, 1, &vertexBindingDesc, 2, vertexAttrDesc);
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
	MSState.rasterizationSamples = (VULKAN_HPP_NAMESPACE::SampleCountFlagBits)window_->sampleCountFlagBits();
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
	piplineLayout_ = device.createPipelineLayout(piplineLayoutInfo);
	piplineInfo.layout = piplineLayout_;

	piplineInfo.renderPass = window_->defaultRenderPass();

	piplineCache_ = device.createPipelineCache(vk::PipelineCacheCreateInfo());
	pipline_ = device.createGraphicsPipeline(piplineCache_, piplineInfo).value;

	device.destroyShaderModule(vertShader);
	device.destroyShaderModule(fragShader);
}

void TextureRenderer::initSwapChainResources()
{
}

void TextureRenderer::releaseSwapChainResources()
{
}

void TextureRenderer::releaseResources() {
	vk::Device device = window_->device();
	device.destroyPipeline(pipline_);
	device.destroyPipelineCache(piplineCache_);
	device.destroyPipelineLayout(piplineLayout_);
	device.destroyDescriptorSetLayout(descSetLayout_);
	device.destroyDescriptorPool(descPool_);
	device.destroyBuffer(vertexBuffer_);
	device.freeMemory(vertexDevMemory_);
	device.destroyBuffer(indexBuffer_);
	device.freeMemory(indexDevMemory_);
	device.destroyBuffer(uniformBuffer_);
	device.freeMemory(uniformDevMemory_);
	device.destroySampler(sampler_);
	device.destroyImage(image_);
	device.freeMemory(imageDevMemory_);
	device.destroyImageView(imageView_);
}

void TextureRenderer::startNextFrame() {
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

	uint8_t* uniformMatrixMemPtr = (uint8_t*)device.mapMemory(uniformDevMemory_, uniformBufferInfo_[window_->currentFrame()].offset, sizeof(float) * 16, {});
	QMatrix4x4 matrix;
	matrix.rotate(QTime::currentTime().msecsSinceStartOfDay() / 10.0, 0, 0, 1);
	memcpy(uniformMatrixMemPtr, matrix.constData(), 16 * sizeof(float));
	device.unmapMemory(uniformDevMemory_);

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

	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, piplineLayout_, 0, 1, &descSet_[window_->currentFrame()], 0, nullptr);

	cmdBuffer.bindVertexBuffers(0, vertexBuffer_, { 0 });

	cmdBuffer.bindIndexBuffer(indexBuffer_, 0, vk::IndexType::eUint16);

	cmdBuffer.drawIndexed(6, 1, 0, 0, 0);

	cmdBuffer.endRenderPass();

	window_->frameReady();
	window_->requestUpdate();
}