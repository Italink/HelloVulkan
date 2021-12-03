#include "VulkanWindow.h"
#include <QFile>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

static float vertexData[] = { // Y up, front = CCW
	 0.0f,  -0.1f,   1.0f, 0.0f, 0.0f,
	-0.1f,   0.1f,   0.0f, 1.0f, 0.0f,
	 0.1f,   0.1f,   0.0f, 0.0f, 1.0f
};

static float instanceData[] = {
	 0.0f,   0.5f,
	-0.5f,  -0.5f,
	 0.5f,  -0.5f,
};

TriangleRenderer::TriangleRenderer(QVulkanWindow* window)
	:window_(window)
{
	QList<int> sampleCounts= window->supportedSampleCounts();
	if (!sampleCounts.isEmpty()) {
		window->setSampleCount(sampleCounts.back());
	}
}

void TriangleRenderer::initResources()
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

	vk::BufferCreateInfo instanceBufferInfo;
	instanceBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
	instanceBufferInfo.size = sizeof(instanceData);
	instanceBuffer_ = device.createBuffer(instanceBufferInfo);
	vk::MemoryRequirements instanceMemReq = device.getBufferMemoryRequirements(instanceBuffer_);
	vk::MemoryAllocateInfo instanceMemAllocInfo(instanceMemReq.size, window_->hostVisibleMemoryIndex());
	instanceDevMemory_ = device.allocateMemory(instanceMemAllocInfo);
	device.bindBufferMemory(instanceBuffer_, instanceDevMemory_, 0);
	uint8_t* instanceBufferMemPtr = (uint8_t*)device.mapMemory(instanceDevMemory_, 0, instanceMemReq.size);
	memcpy(instanceBufferMemPtr, instanceData, sizeof(instanceData));
	device.unmapMemory(instanceDevMemory_);


	vk::DescriptorPoolSize descPoolSize(vk::DescriptorType::eUniformBuffer, (uint32_t)concurrentFrameCount);
	vk::DescriptorPoolCreateInfo descPoolInfo;
	descPoolInfo.maxSets = concurrentFrameCount;
	descPoolInfo.poolSizeCount = 1;
	descPoolInfo.pPoolSizes = &descPoolSize;
	descPool_ = device.createDescriptorPool(descPoolInfo);

	vk::DescriptorSetLayoutBinding layoutBinding;
	layoutBinding.binding = 0;
	layoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
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
	}

	vk::GraphicsPipelineCreateInfo piplineInfo;
	piplineInfo.stageCount = 2;

	vk::ShaderModule vertShader = loadShader("./color_vert.spv");
	vk::ShaderModule fragShader = loadShader("./color_frag.spv");

	vk::PipelineShaderStageCreateInfo piplineShaderStage[2] = {
		{
			{},
			vk::ShaderStageFlagBits::eVertex,
			vertShader,
			"main",
			nullptr
		},
		{
			{},
			vk::ShaderStageFlagBits::eFragment,
			fragShader,
			"main",
			nullptr
		}
	};

	piplineInfo.pStages = piplineShaderStage;

	std::array<vk::VertexInputBindingDescription,2> vertexBindingDesc = {
		vk::VertexInputBindingDescription{0, 5 * sizeof(float), vk::VertexInputRate::eVertex},
		vk::VertexInputBindingDescription{1, 2 * sizeof(float), vk::VertexInputRate::eInstance}
	};

	std::array<vk::VertexInputAttributeDescription,3> vertexAttrDesc ={
		vk::VertexInputAttributeDescription{ 0,0,vk::Format::eR32G32Sfloat,0 },
		vk::VertexInputAttributeDescription{ 1,0,vk::Format::eR32G32B32Sfloat,2 * sizeof(float) },
		vk::VertexInputAttributeDescription{ 2,1,vk::Format::eR32G32Sfloat,0}
	};

	vk::PipelineVertexInputStateCreateInfo vertexInputState({},vertexBindingDesc,vertexAttrDesc);
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

void TriangleRenderer::initSwapChainResources()
{

}

void TriangleRenderer::releaseSwapChainResources()
{

}

void TriangleRenderer::releaseResources(){
	vk::Device device = window_->device();
	device.destroyPipeline(pipline_);
	device.destroyPipelineCache(piplineCache_);
	device.destroyPipelineLayout(piplineLayout_);
	device.destroyDescriptorSetLayout(descSetLayout_);
	device.destroyDescriptorPool(descPool_);
	device.destroyBuffer(vertexBuffer_);
	device.freeMemory(vertexDevMemory_);
	device.destroyBuffer(instanceBuffer_);
	device.freeMemory(instanceDevMemory_);
}

void TriangleRenderer::startNextFrame(){
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
	cmdBuffer.bindVertexBuffers(1, instanceBuffer_, { 0 });

	cmdBuffer.draw(3, std::size(instanceData)/2, 0, 0);

	cmdBuffer.endRenderPass();

	window_->frameReady();
	window_->requestUpdate();
}

vk::ShaderModule TriangleRenderer::loadShader(const QString& name)
{
	QFile file(name);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning("Failed to read shader %s", qPrintable(name));
		return nullptr;
	}
	QByteArray blob = file.readAll();
	file.close();
	vk::ShaderModuleCreateInfo shaderInfo;
	shaderInfo.codeSize = blob.size();
	shaderInfo.pCode = reinterpret_cast<const uint32_t*>(blob.constData());
	vk::Device device = window_->device();
	return device.createShaderModule(shaderInfo);
}