#include "VulkanWindow.h"
#include <QFile>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

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

TriangleRenderer::TriangleRenderer(QVulkanWindow* window)
	:window_(window)
{
	QList<int> sampleCounts = window->supportedSampleCounts();
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

	vk::GraphicsPipelineCreateInfo piplineInfo;
	piplineInfo.stageCount = 2;

	vk::ShaderModule vertShader = loadShader("./color_vert.spv");
	vk::ShaderModule fragShader = loadShader("./color_frag.spv");

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
	colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	colorBlendState.pAttachments = &colorBlendAttachmentState;
	piplineInfo.pColorBlendState = &colorBlendState;

	vk::PipelineDynamicStateCreateInfo dynamicState;
	vk::DynamicState dynamicEnables[] = { vk::DynamicState::eViewport ,vk::DynamicState::eScissor };
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicEnables;
	piplineInfo.pDynamicState = &dynamicState;

	vk::PipelineLayoutCreateInfo piplineLayoutInfo;
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

void TriangleRenderer::releaseResources() {
	vk::Device device = window_->device();
	device.destroyPipeline(pipline_);
	device.destroyPipelineCache(piplineCache_);
	device.destroyPipelineLayout(piplineLayout_);
	device.destroyBuffer(vertexBuffer_);
	device.freeMemory(vertexDevMemory_);
	device.destroyBuffer(indexBuffer_);
	device.freeMemory(indexDevMemory_);
}

void TriangleRenderer::startNextFrame() {
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

	cmdBuffer.bindVertexBuffers(0, vertexBuffer_, { 0 });

	cmdBuffer.bindIndexBuffer(indexBuffer_, 0, vk::IndexType::eUint16);

	cmdBuffer.drawIndexed(6, 1, 0, 0, 0);

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