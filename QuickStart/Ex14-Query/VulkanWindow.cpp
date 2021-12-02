#include "VulkanWindow.h"
#include <QFile>
#include <QTime>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

static float vertexData[] = { // Y up, front = CCW
	 0.0f,   0.5f,   1.0f, 0.0f, 0.0f,
	-0.5f,  -0.5f,   0.0f, 1.0f, 0.0f,
	 0.5f,  -0.5f,   0.0f, 0.0f, 1.0f
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

	vk::VertexInputBindingDescription vertexBindingDesc(0, 5 * sizeof(float), vk::VertexInputRate::eVertex);

	vk::VertexInputAttributeDescription vertexAttrDesc[] = {
		{0,0,vk::Format::eR32G32Sfloat,0},
		{1,0,vk::Format::eR32G32B32Sfloat,2 * sizeof(float)}
	};
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
	piplineLayoutInfo.setLayoutCount = 1;
	piplineLayoutInfo.pSetLayouts = &descSetLayout_;
	pushConstant_.size = sizeof(float);
	pushConstant_.stageFlags = vk::ShaderStageFlagBits::eVertex;
	piplineLayoutInfo.pushConstantRangeCount = 1;
	piplineLayoutInfo.pPushConstantRanges = &pushConstant_;

	piplineLayout_ = device.createPipelineLayout(piplineLayoutInfo);
	piplineInfo.layout = piplineLayout_;

	piplineInfo.renderPass = window_->defaultRenderPass();

	piplineCache_ = device.createPipelineCache(vk::PipelineCacheCreateInfo());
	pipline_ = device.createGraphicsPipeline(piplineCache_, piplineInfo).value;

	device.destroyShaderModule(vertShader);
	device.destroyShaderModule(fragShader);

	vk::QueryPoolCreateInfo queryPoolInfo;
	queryPoolInfo.queryType = vk::QueryType::eOcclusion;
	queryPoolInfo.queryCount = 1;
	occlusionQueryPool_ = device.createQueryPool(queryPoolInfo);

	queryPoolInfo.queryType = vk::QueryType::eTimestamp;
	queryPoolInfo.queryCount = 2;
	timestampQueryPool_ = device.createQueryPool(queryPoolInfo);

	queryPoolInfo.queryType = vk::QueryType::ePipelineStatistics;
	queryPoolInfo.pipelineStatistics = vk::QueryPipelineStatisticFlagBits::eInputAssemblyVertices |
									   vk::QueryPipelineStatisticFlagBits::eInputAssemblyPrimitives |
									   vk::QueryPipelineStatisticFlagBits::eVertexShaderInvocations |
									   vk::QueryPipelineStatisticFlagBits::eClippingInvocations |
									   vk::QueryPipelineStatisticFlagBits::eClippingPrimitives |
									   vk::QueryPipelineStatisticFlagBits::eFragmentShaderInvocations ;
	queryPoolInfo.queryCount = 6;
	piplineQueryPool_ = device.createQueryPool(queryPoolInfo);
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
	device.destroyDescriptorSetLayout(descSetLayout_);
	device.destroyDescriptorPool(descPool_);
	device.destroyBuffer(vertexBuffer_);
	device.freeMemory(vertexDevMemory_);
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

	cmdBuffer.resetQueryPool(occlusionQueryPool_, 0, 1);
	cmdBuffer.resetQueryPool(timestampQueryPool_, 0, 2);
	cmdBuffer.resetQueryPool(piplineQueryPool_, 0, 6);
	// device.resetQueryPool(queryPool_,0,1);

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

	cmdBuffer.writeTimestamp(vk::PipelineStageFlagBits::eFragmentShader, timestampQueryPool_, 0);

	cmdBuffer.pushConstants<float>(piplineLayout_, pushConstant_.stageFlags, 0, QTime::currentTime().msecsSinceStartOfDay() / 1000.0f);

	cmdBuffer.beginQuery(occlusionQueryPool_, 0, {});

	cmdBuffer.beginQuery(piplineQueryPool_,0,{});

	cmdBuffer.draw(3, 1, 0, 0);

	cmdBuffer.endQuery(piplineQueryPool_, 0);

	cmdBuffer.endQuery(occlusionQueryPool_, 0);

	cmdBuffer.writeTimestamp(vk::PipelineStageFlagBits::eFragmentShader, timestampQueryPool_, 1);

	cmdBuffer.endRenderPass();


	window_->frameReady();

	uint64_t result;



	device.getQueryPoolResults(occlusionQueryPool_, 0, 1, sizeof(uint64_t), &result, sizeof(uint64_t), vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait);
	qDebug() << "Occlusion Query :  " << result;

	uint64_t time[2];
	device.getQueryPoolResults(timestampQueryPool_, 0, 2, sizeof(time), time, sizeof(uint64_t), vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait);
	qDebug() << "Timestamp Query FragmentShader Stage : " << time[1] - time[0] <<" -- start : " << time[0];

	uint64_t pipline[6];
	device.getQueryPoolResults(piplineQueryPool_, 0, 1, sizeof(pipline), pipline, sizeof(uint64_t), vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait);
	qDebug() << "Pipeline Statistics Query : \n"
		<< "     Input assembly vertex count :         " << pipline[0] << "\n"
		<< "     Input assembly primitives count :     " << pipline[1] << "\n"
		<< "     Vertex shader invocations :           " << pipline[2] << "\n"
		<< "     Clipping stage primitives processed : " << pipline[3] << "\n"
		<< "     Clipping stage primitives output :    " << pipline[4] << "\n"
		<< "     Fragment shader invocations           " << pipline[5] << "\n"
			 ;

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