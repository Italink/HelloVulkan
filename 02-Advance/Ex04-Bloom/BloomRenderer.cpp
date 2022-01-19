#include "BloomRenderer.h"
#include "full_screen_vert.inl"
#include "gaussblur_frag.inl"

BloomRenderer::BloomRenderer()
{
	setBlurSize(20);
	setBlurStrength(10);
}

void BloomRenderer::setBlurSize(int size)
{
	if (size <= 0 || size == blurParams_.size || size >= std::size(blurParams_.weight))
		return;
	float sum = 0, s = 1;
	for (int i = size - 1; i >= 0; i--) {
		blurParams_.weight[i] = (blurParams_.weight[0] + s);
		++s;
		sum += blurParams_.weight[i] * 2;
	}
	for (int i = 0; i < size; i++) {
		blurParams_.weight[i] /= sum;
	}
}

void BloomRenderer::setBlurStrength(float strength)
{
	blurParams_.strength = strength;
}

void BloomRenderer::setBlurScale(float scale)
{
	blurParams_.scale = scale;
}

void BloomRenderer::initResources()
{
	vk::Device device = window_->device();
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eNearest;
	samplerInfo.minFilter = vk::Filter::eNearest;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.maxAnisotropy = 1.0f;
	sampler_ = device.createSampler(samplerInfo);

	vk::AttachmentDescription attachmentDesc;
	attachmentDesc.format = vk::Format::eR8G8B8A8Unorm;
	attachmentDesc.samples = vk::SampleCountFlagBits::e1;
	attachmentDesc.loadOp = vk::AttachmentLoadOp::eClear;
	attachmentDesc.storeOp = vk::AttachmentStoreOp::eStore;
	attachmentDesc.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachmentDesc.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachmentDesc.initialLayout = vk::ImageLayout::eUndefined;
	attachmentDesc.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference attachmentRef;
	attachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
	attachmentRef.attachment = 0;
	vk::SubpassDescription subpassDesc;
	subpassDesc.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpassDesc.colorAttachmentCount = 1;
	subpassDesc.pColorAttachments = &attachmentRef;

	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &attachmentDesc;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDesc;
	renderPass_ = device.createRenderPass(renderPassInfo);

	vk::DescriptorPoolSize descPoolSize = {
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 2)
	};

	vk::DescriptorPoolCreateInfo descPoolInfo;
	descPoolInfo.maxSets = 2;
	descPoolInfo.poolSizeCount = 1;
	descPoolInfo.pPoolSizes = &descPoolSize;
	descPool_ = device.createDescriptorPool(descPoolInfo);

	vk::DescriptorSetLayoutBinding layoutBinding{
		0, vk::DescriptorType::eCombinedImageSampler,1,vk::ShaderStageFlagBits::eFragment
	};

	vk::DescriptorSetLayoutCreateInfo descLayoutInfo;
	descLayoutInfo.pNext = nullptr;
	descLayoutInfo.flags = {};
	descLayoutInfo.bindingCount = 1;
	descLayoutInfo.pBindings = &layoutBinding;

	descSetLayout_ = device.createDescriptorSetLayout(descLayoutInfo);

	for (int i = 0; i < 2; ++i) {
		vk::DescriptorSetAllocateInfo descSetAllocInfo(descPool_, 1, &descSetLayout_);
		descSet_[i] = device.allocateDescriptorSets(descSetAllocInfo).front();
	}

	vk::GraphicsPipelineCreateInfo piplineInfo;

	vk::ShaderModuleCreateInfo shaderInfo;
	shaderInfo.codeSize = sizeof(full_screen_vert);
	shaderInfo.pCode = full_screen_vert;
	vk::ShaderModule vertShader = device.createShaderModule(shaderInfo);

	shaderInfo.codeSize = sizeof(gaussblur_frag);
	shaderInfo.pCode = gaussblur_frag;
	vk::ShaderModule fragShader = device.createShaderModule(shaderInfo);

	vk::SpecializationMapEntry specMapEntry;
	specMapEntry.constantID = 0;
	specMapEntry.offset = 0;
	specMapEntry.size = sizeof(int);
	vk::SpecializationInfo specInfo;

	int specData = 0;
	specInfo.dataSize = sizeof(int);
	specInfo.mapEntryCount = 1;
	specInfo.pMapEntries = &specMapEntry;
	specInfo.pData = &specData;

	vk::PipelineShaderStageCreateInfo piplineShaderStage[2];
	piplineShaderStage[0].stage = vk::ShaderStageFlagBits::eVertex;
	piplineShaderStage[0].module = vertShader;
	piplineShaderStage[0].pName = "main";
	piplineShaderStage[1].stage = vk::ShaderStageFlagBits::eFragment;
	piplineShaderStage[1].module = fragShader;
	piplineShaderStage[1].pName = "main";

	piplineInfo.stageCount = 2;
	piplineInfo.pStages = piplineShaderStage;

	vk::PipelineVertexInputStateCreateInfo vertexInputState({}, 0, nullptr, 0, nullptr);
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

	MSState.rasterizationSamples = vk::SampleCountFlagBits::e1;
	piplineInfo.pMultisampleState = &MSState;

	vk::PipelineDepthStencilStateCreateInfo DSState;
	DSState.depthTestEnable = false;
	DSState.depthWriteEnable = false;
	DSState.depthCompareOp = vk::CompareOp::eLessOrEqual;
	piplineInfo.pDepthStencilState = &DSState;

	vk::PipelineColorBlendStateCreateInfo colorBlendState;
	colorBlendState.attachmentCount = 1;
	vk::PipelineColorBlendAttachmentState colorBlendAttachmentState;
	colorBlendAttachmentState.blendEnable = false;
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

	pushConstant_.offset = 0;
	pushConstant_.stageFlags = vk::ShaderStageFlagBits::eFragment;
	pushConstant_.size = sizeof(BlurParams);

	vk::PipelineLayoutCreateInfo piplineLayoutInfo;
	piplineLayoutInfo.setLayoutCount = 1;
	piplineLayoutInfo.pSetLayouts = &descSetLayout_;
	piplineLayoutInfo.pushConstantRangeCount = 1;
	piplineLayoutInfo.pPushConstantRanges = &pushConstant_;

	piplineLayout_ = device.createPipelineLayout(piplineLayoutInfo);

	piplineInfo.layout = piplineLayout_;

	piplineInfo.renderPass = renderPass_;

	piplineCache_ = device.createPipelineCache(vk::PipelineCacheCreateInfo());

	hBlurPipline_ = device.createGraphicsPipeline(piplineCache_, piplineInfo).value;

	specData = 1;
	vBlurPipline_ = device.createGraphicsPipeline(piplineCache_, piplineInfo).value;

	device.destroyShaderModule(vertShader);
	device.destroyShaderModule(fragShader);

	textureRenderer.setWindow(window_);
	textureRenderer.initResources();
}

void BloomRenderer::initSwapChainResources() {
	vk::Device device = window_->device();
	vk::ImageCreateInfo imageInfo;
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.extent.width = window_->swapChainImageSize().width();
	imageInfo.extent.height = window_->swapChainImageSize().height();
	imageInfo.extent.depth = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.mipLevels = 1;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	imageInfo.format = vk::Format::eR8G8B8A8Unorm;
	imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment;
	imageInfo.tiling = vk::ImageTiling::eOptimal;
	imageInfo.initialLayout = vk::ImageLayout::ePreinitialized;
	frameBuffer_[0].image = device.createImage(imageInfo);
	frameBuffer_[1].image = device.createImage(imageInfo);
	vk::MemoryRequirements memReq = device.getImageMemoryRequirements(frameBuffer_[0].image);
	vk::MemoryAllocateInfo memAllocInfo(memReq.size, window_->deviceLocalMemoryIndex());
	frameBuffer_[0].imageMemory = device.allocateMemory(memAllocInfo);
	frameBuffer_[1].imageMemory = device.allocateMemory(memAllocInfo);

	device.bindImageMemory(frameBuffer_[0].image, frameBuffer_[0].imageMemory, 0);
	device.bindImageMemory(frameBuffer_[1].image, frameBuffer_[1].imageMemory, 0);

	vk::ImageViewCreateInfo imageViewInfo;
	imageViewInfo.viewType = vk::ImageViewType::e2D;
	imageViewInfo.components = { vk::ComponentSwizzle::eR,vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA };
	imageViewInfo.format = imageInfo.format;
	imageViewInfo.image = frameBuffer_[0].image;
	imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	imageViewInfo.subresourceRange.layerCount = imageViewInfo.subresourceRange.levelCount = 1;
	frameBuffer_[0].imageView = device.createImageView(imageViewInfo);

	imageViewInfo.image = frameBuffer_[1].image;
	frameBuffer_[1].imageView = device.createImageView(imageViewInfo);

	vk::FramebufferCreateInfo framebufferInfo;
	framebufferInfo.renderPass = renderPass_;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = &frameBuffer_[0].imageView;
	framebufferInfo.width = window_->swapChainImageSize().width();
	framebufferInfo.height = window_->swapChainImageSize().height();
	framebufferInfo.layers = 1;
	frameBuffer_[0].framebuffer = device.createFramebuffer(framebufferInfo);

	framebufferInfo.pAttachments = &frameBuffer_[1].imageView;
	frameBuffer_[1].framebuffer = device.createFramebuffer(framebufferInfo);

	vk::ImageMemoryBarrier barrier;
	barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
	barrier.oldLayout = vk::ImageLayout::ePreinitialized;
	barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.image = frameBuffer_[0].image;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	vk::CommandBufferAllocateInfo cmdBufferAlllocInfo;
	cmdBufferAlllocInfo.level = vk::CommandBufferLevel::ePrimary;
	cmdBufferAlllocInfo.commandPool = window_->graphicsCommandPool();
	cmdBufferAlllocInfo.commandBufferCount = 1;
	vk::CommandBuffer cmdBuffer = device.allocateCommandBuffers(cmdBufferAlllocInfo).front();
	vk::CommandBufferBeginInfo cmdBufferBeginInfo;
	cmdBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue;
	cmdBuffer.begin(cmdBufferBeginInfo);
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);
	barrier.image = frameBuffer_[1].image;
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);
	cmdBuffer.end();

	vk::Queue graphicsQueue = window_->graphicsQueue();
	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	graphicsQueue.submit(submitInfo);
	graphicsQueue.waitIdle();
	device.freeCommandBuffers(window_->graphicsCommandPool(), cmdBuffer);

	for (int i = 0; i < 2; ++i) {
		vk::WriteDescriptorSet descWrite;
		vk::DescriptorImageInfo descImageInfo(sampler_, frameBuffer_[i].imageView, vk::ImageLayout::eShaderReadOnlyOptimal);
		descWrite.dstSet = descSet_[i];
		descWrite.dstBinding = 0;
		descWrite.descriptorCount = 1;
		descWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descWrite.pImageInfo = &descImageInfo;
		device.updateDescriptorSets(1, &descWrite, 0, nullptr);
	}
	textureRenderer.updateImage(frameBuffer_[0].imageView);
}

void BloomRenderer::releaseSwapChainResources()
{
	vk::Device device = window_->device();
	for (int i = 0; i < std::size(frameBuffer_); i++) {
		device.freeMemory(frameBuffer_[i].imageMemory);
		device.destroyImageView(frameBuffer_[i].imageView);
		device.destroyImage(frameBuffer_[i].image);
		device.destroyFramebuffer(frameBuffer_[i].framebuffer);
	}
}

void BloomRenderer::releaseResources()
{
	vk::Device device = window_->device();
	device.destroySampler(sampler_);
	device.destroyRenderPass(renderPass_);
	device.destroyDescriptorPool(descPool_);
	device.destroyDescriptorSetLayout(descSetLayout_);
	device.destroyPipeline(hBlurPipline_);
	device.destroyPipeline(vBlurPipline_);
	device.destroyPipelineCache(piplineCache_);
	device.destroyPipelineLayout(piplineLayout_);
	textureRenderer.releaseResources();
}

void BloomRenderer::startNextFrame()
{
	vk::Device device = window_->device();
	vk::CommandBuffer cmdBuffer = window_->currentCommandBuffer();
	vk::Image currentImage = window_->swapChainImage(window_->currentSwapChainImageIndex());
	vk::ImageMemoryBarrier barrier;
	barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
	barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
	barrier.oldLayout = vk::ImageLayout::ePresentSrcKHR;
	barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
	barrier.image = currentImage;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

	barrier.oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.image = frameBuffer_[0].image;
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

	vk::ImageCopy imageCopy;
	imageCopy.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	imageCopy.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	imageCopy.srcSubresource.layerCount = 1;
	imageCopy.dstSubresource.layerCount = 1;

	imageCopy.extent.width = window_->swapChainImageSize().width();
	imageCopy.extent.height = window_->swapChainImageSize().height();
	imageCopy.extent.depth = 1;
	cmdBuffer.copyImage(currentImage, vk::ImageLayout::eTransferSrcOptimal, frameBuffer_[0].image, vk::ImageLayout::eTransferDstOptimal, imageCopy);

	vk::ClearValue clearValues[3] = {
		vk::ClearColorValue(std::array<float,4>{1.0f,0.0f,0.0f,1.0f }),
		vk::ClearDepthStencilValue(1.0f,0),
	};

	barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.image = frameBuffer_[0].image;
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

	vk::RenderPassBeginInfo beginInfo;
	beginInfo.renderPass = renderPass_;
	beginInfo.framebuffer = frameBuffer_[1].framebuffer;
	beginInfo.renderArea.extent.width = window_->swapChainImageSize().width();
	beginInfo.renderArea.extent.height = window_->swapChainImageSize().height();
	beginInfo.clearValueCount = 2;
	beginInfo.pClearValues = clearValues;
	cmdBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);
	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, hBlurPipline_);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, piplineLayout_, 0, 1, &descSet_[0], 0, nullptr);
	cmdBuffer.pushConstants<BlurParams>(piplineLayout_, pushConstant_.stageFlags, 0, blurParams_);
	cmdBuffer.draw(4, 1, 0, 0);
	cmdBuffer.endRenderPass();

	barrier.oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
	barrier.image = frameBuffer_[0].image;
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

	barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
	barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.image = frameBuffer_[1].image;
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

	beginInfo.framebuffer = frameBuffer_[0].framebuffer;
	cmdBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);
	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vBlurPipline_);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, piplineLayout_, 0, 1, &descSet_[1], 0, nullptr);
	cmdBuffer.pushConstants<BlurParams>(piplineLayout_, pushConstant_.stageFlags, 0, blurParams_);
	cmdBuffer.draw(4, 1, 0, 0);
	cmdBuffer.endRenderPass();

	barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
	barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
	barrier.image = currentImage;
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

	barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
	barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.image = frameBuffer_[0].image;
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

	textureRenderer.startNextFrame();
}