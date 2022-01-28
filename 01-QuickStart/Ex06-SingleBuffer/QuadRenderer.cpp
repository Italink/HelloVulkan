#include "QuadRenderer.h"
#include <QTime>
#include "Shaders/quad_vert.inl"
#include "Shaders/quad_frag.inl"

static float vertexData[] = { // Y up, front = CCW
	-0.5f, -0.5f,		 1.0f, 0.0f, 0.0f,
	 0.5f, -0.5f,		 0.0f, 1.0f, 0.0f,
	 0.5f,  0.5f,		 0.0f, 0.0f, 1.0f,
	-0.5f,  0.5f,		 1.0f, 1.0f, 1.0f
};

static uint16_t indexData[] = {
	0,1,2,
	2,3,0
};

static inline vk::DeviceSize aligned(vk::DeviceSize v, vk::DeviceSize byteAlign) {
	return (v + byteAlign - 1) & ~(byteAlign - 1);
}

QuadRenderer::QuadRenderer(QVulkanWindow* window)
	:window_(window)
{
	QList<int> sampleCounts = window->supportedSampleCounts();
	if (!sampleCounts.isEmpty()) {
		window->setSampleCount(sampleCounts.back());
	}
}

void QuadRenderer::initResources()
{
	vk::Device device = window_->device();

	const int concurrentFrameCount = window_->concurrentFrameCount();
	vk::PhysicalDeviceLimits limits = window_->physicalDeviceProperties()->limits;

	vertexBufferInfo_.buffer = indexBufferInfo_.buffer = singleBuffer_;
	vertexBufferInfo_.range = aligned(sizeof(vertexData), limits.minUniformBufferOffsetAlignment);

	indexBufferInfo_.offset = vertexBufferInfo_.range;
	indexBufferInfo_.range = aligned(sizeof(indexData), limits.minUniformBufferOffsetAlignment);

	vk::DeviceSize uniformAllocSize_ = aligned(sizeof(float) * 16, limits.minUniformBufferOffsetAlignment);

	vk::BufferCreateInfo singleBufferInfo;
	singleBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eUniformBuffer;
	singleBufferInfo.size = vertexBufferInfo_.range + indexBufferInfo_.range + uniformAllocSize_ * concurrentFrameCount;
	singleBuffer_ = device.createBuffer(singleBufferInfo);
	vk::MemoryRequirements singleMemReq = device.getBufferMemoryRequirements(singleBuffer_);
	vk::MemoryAllocateInfo singleMemAllocInfo(singleMemReq.size, window_->hostVisibleMemoryIndex());
	singleDevMemory_ = device.allocateMemory(singleMemAllocInfo);
	device.bindBufferMemory(singleBuffer_, singleDevMemory_, 0);
	uint8_t* singleBufferMemPtr = (uint8_t*)device.mapMemory(singleDevMemory_, 0, singleMemReq.size);
	memcpy(singleBufferMemPtr, vertexData, sizeof(vertexData));
	memcpy(singleBufferMemPtr + vertexBufferInfo_.range, indexData, sizeof(indexData));
	QMatrix4x4 identify;
	for (int i = 0; i < concurrentFrameCount; i++) {
		vk::DeviceSize offset = vertexBufferInfo_.range + indexBufferInfo_.range + i * uniformAllocSize_;
		memcpy(singleBufferMemPtr + offset, identify.constData(), 16 * sizeof(float));
		uniformBufferInfo_[i].buffer = singleBuffer_;
		uniformBufferInfo_[i].offset = offset;
		uniformBufferInfo_[i].range = uniformAllocSize_;
	}
	device.unmapMemory(singleDevMemory_);

	vk::DescriptorPoolSize descPoolSize(vk::DescriptorType::eUniformBuffer, (uint32_t)concurrentFrameCount);
	vk::DescriptorPoolCreateInfo descPoolInfo;
	descPoolInfo.maxSets = concurrentFrameCount;
	descPoolInfo.poolSizeCount = 1;
	descPoolInfo.pPoolSizes = &descPoolSize;
	descPool_ = device.createDescriptorPool(descPoolInfo);

	vk::DescriptorSetLayoutBinding layoutBinding;
	layoutBinding.binding = 0;
	layoutBinding.descriptorType = descPoolSize.type;
	layoutBinding.descriptorCount = 1;
	layoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
	layoutBinding.pImmutableSamplers = nullptr;

	vk::DescriptorSetLayoutCreateInfo descLayoutInfo;
	descLayoutInfo.pNext = nullptr;
	descLayoutInfo.flags = {};
	descLayoutInfo.bindingCount = 1;
	descLayoutInfo.pBindings = &layoutBinding;

	descSetLayout_ = device.createDescriptorSetLayout(descLayoutInfo);

	for (int i = 0; i < concurrentFrameCount; ++i) {
		vk::DescriptorSetAllocateInfo descSetAllocInfo(descPool_, 1, &descSetLayout_);
		descSet_[i] = device.allocateDescriptorSets(descSetAllocInfo).front();
		vk::WriteDescriptorSet descWrite;
		descWrite.dstSet = descSet_[i];
		descWrite.descriptorCount = 1;
		descWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		descWrite.pBufferInfo = &uniformBufferInfo_[i];
		device.updateDescriptorSets(1, &descWrite, 0, nullptr);
	}

	vk::GraphicsPipelineCreateInfo piplineInfo;
	piplineInfo.stageCount = 2;

	vk::ShaderModuleCreateInfo shaderInfo;
	shaderInfo.codeSize = sizeof(quad_vert);
	shaderInfo.pCode = quad_vert;
	vk::ShaderModule vertShader = device.createShaderModule(shaderInfo);

	shaderInfo.codeSize = sizeof(quad_frag);
	shaderInfo.pCode = quad_frag;
	vk::ShaderModule fragShader = device.createShaderModule(shaderInfo);

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
	vertexBindingDesc.stride = 5 * sizeof(float);
	vertexBindingDesc.inputRate = vk::VertexInputRate::eVertex;

	vk::VertexInputAttributeDescription vertexAttrDesc[2];
	vertexAttrDesc[0].binding = 0;
	vertexAttrDesc[0].location = 0;
	vertexAttrDesc[0].format = vk::Format::eR32G32Sfloat;
	vertexAttrDesc[0].offset = 0;
	vertexAttrDesc[1].binding = 0;
	vertexAttrDesc[1].location = 1;
	vertexAttrDesc[1].format = vk::Format::eR32G32B32Sfloat;
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

void QuadRenderer::initSwapChainResources()
{
}

void QuadRenderer::releaseSwapChainResources()
{
}

void QuadRenderer::releaseResources() {
	vk::Device device = window_->device();
	device.destroyPipeline(pipline_);
	device.destroyPipelineCache(piplineCache_);
	device.destroyPipelineLayout(piplineLayout_);
	device.destroyDescriptorSetLayout(descSetLayout_);
	device.destroyDescriptorPool(descPool_);
	device.destroyBuffer(singleBuffer_);
	device.freeMemory(singleDevMemory_);
}

void QuadRenderer::startNextFrame() {
	vk::Device device = window_->device();
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

	uint8_t* uniformMatrixMemPtr = (uint8_t*)device.mapMemory(singleDevMemory_, uniformBufferInfo_[window_->currentFrame()].offset, sizeof(float) * 16, {});
	QMatrix4x4 matrix;
	matrix.rotate(QTime::currentTime().msecsSinceStartOfDay() / 10.0, 0, 0, 1);
	memcpy(uniformMatrixMemPtr, matrix.constData(), 16 * sizeof(float));
	device.unmapMemory(singleDevMemory_);

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

	cmdBuffer.bindVertexBuffers(0, singleBuffer_, { 0 });

	cmdBuffer.bindIndexBuffer(singleBuffer_, indexBufferInfo_.offset, vk::IndexType::eUint16);

	cmdBuffer.drawIndexed(6, 1, 0, 0, 0);

	cmdBuffer.endRenderPass();

	window_->frameReady();
	window_->requestUpdate();
}