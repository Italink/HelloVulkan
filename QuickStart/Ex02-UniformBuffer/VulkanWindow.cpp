#include "VulkanWindow.h"
#include <QFile>
#include <QTime>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

static float vertexData[] = { 
   //position			color	
	 0.0f, 0.5f,		1.0f, 0.0f, 0.0f,
	-0.5f,-0.5f,		0.0f, 1.0f, 0.0f,
	 0.5f,-0.5f,		0.0f, 0.0f, 1.0f
};

static inline vk::DeviceSize aligned(vk::DeviceSize v, vk::DeviceSize byteAlign) {
	return (v + byteAlign - 1) & ~(byteAlign - 1);
}

TextureRenderer::TextureRenderer(QVulkanWindow* window)
	:mWindow(window)
{
	QList<int> sampleCounts= window->supportedSampleCounts();
	if (!sampleCounts.isEmpty()) {
		window->setSampleCount(sampleCounts.back());
	}
}

void TextureRenderer::initResources()
{
	vk::Device device = mWindow->device();

	const int concurrentFrameCount = mWindow->concurrentFrameCount();

	vk::PhysicalDeviceLimits limits = mWindow->physicalDeviceProperties()->limits;

	vk::DeviceSize vertexAllocSize = aligned(sizeof(vertexData), limits.minUniformBufferOffsetAlignment);
	vk::BufferCreateInfo vertexBufferInfo;
	vertexBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
	vertexBufferInfo.size = vertexAllocSize;
	vertexBuffer_ = device.createBuffer(vertexBufferInfo);
	vk::MemoryRequirements vertexMemReq = device.getBufferMemoryRequirements(vertexBuffer_);
	vk::MemoryAllocateInfo vertexMemAllocInfo(vertexMemReq.size, mWindow->hostVisibleMemoryIndex());
	vertexDevMemory_ = device.allocateMemory(vertexMemAllocInfo);
	device.bindBufferMemory(vertexBuffer_, vertexDevMemory_, 0);
	uint8_t* vertexBufferMemPtr = (uint8_t*)device.mapMemory(vertexDevMemory_, 0, vertexMemReq.size);
	memcpy(vertexBufferMemPtr, vertexData, sizeof(vertexData));
	device.unmapMemory(vertexDevMemory_);

	vk::DeviceSize uniformAllocSize = aligned(16 * sizeof(float), limits.minUniformBufferOffsetAlignment);
	vk::BufferCreateInfo uniformBufferInfo;
	uniformBufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	uniformBufferInfo.size = uniformAllocSize * concurrentFrameCount;
	uniformBuffer_ = device.createBuffer(uniformBufferInfo);
	vk::MemoryRequirements uniformMemReq = device.getBufferMemoryRequirements(uniformBuffer_);
	vk::MemoryAllocateInfo uniformMemAllocInfo(uniformMemReq.size, mWindow->hostVisibleMemoryIndex());
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

	vk::VertexInputBindingDescription vertexBindingDesc(0, 5 * sizeof(float), vk::VertexInputRate::eVertex);

	vk::VertexInputAttributeDescription vertexAttrDesc[] = {
		{0,0,vk::Format::eR32G32Sfloat,0},
		{1,0,vk::Format::eR32G32Sfloat,2 * sizeof(float)}
	};

	vk::PipelineVertexInputStateCreateInfo vertexInputState({}, 1, &vertexBindingDesc, 2, vertexAttrDesc);

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
		vk::WriteDescriptorSet descWrite;
		descWrite.dstSet = descSet_[i];
		descWrite.descriptorCount = 1;
		descWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		descWrite.pBufferInfo = &uniformBufferInfo_[i];
		device.updateDescriptorSets(1, &descWrite, 0, nullptr);
	}

	piplineCache_ = device.createPipelineCache(vk::PipelineCacheCreateInfo());

	vk::PipelineLayoutCreateInfo piplineLayoutInfo;
	piplineLayoutInfo.setLayoutCount = 1;
	piplineLayoutInfo.pSetLayouts = &descSetLayout_;
	piplineLayout_ = device.createPipelineLayout(piplineLayoutInfo);

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
	MSState.rasterizationSamples = (VULKAN_HPP_NAMESPACE::SampleCountFlagBits)mWindow->sampleCountFlagBits();
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

	piplineInfo.layout = piplineLayout_;
	piplineInfo.renderPass = mWindow->defaultRenderPass();

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

void TextureRenderer::releaseResources()
{
}

void TextureRenderer::startNextFrame(){
	vk::Device device = mWindow->device();
	vk::CommandBuffer cmdBuffer = mWindow->currentCommandBuffer();
	const QSize size = mWindow->swapChainImageSize();

	vk::ClearValue clearValues[3] = {
		vk::ClearColorValue(std::array<float,4>{1.0f,0.0f,0.0f,1.0f }),
		vk::ClearDepthStencilValue(1.0f, 0),
		vk::ClearColorValue(std::array<float,4>{ 0.0f,0.5f,0.9f,1.0f }),
	};

	vk::RenderPassBeginInfo beginInfo;
	beginInfo.renderPass = mWindow->defaultRenderPass();
	beginInfo.framebuffer = mWindow->currentFramebuffer();
	beginInfo.renderArea.extent.width = size.width();
	beginInfo.renderArea.extent.height = size.height();
	beginInfo.clearValueCount = mWindow->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
	beginInfo.pClearValues = clearValues;

	cmdBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

	uint8_t* uniformMatrixMemPtr = (uint8_t*)device.mapMemory(uniformDevMemory_, uniformBufferInfo_[mWindow->currentFrame()].offset, sizeof(float) * 16, {});
	QMatrix4x4 matrix; 
	matrix.rotate(QTime::currentTime().msecsSinceStartOfDay()/10.0, 0, 1, 0);
	memcpy(uniformMatrixMemPtr, matrix.constData(), 16 * sizeof(float));
	device.unmapMemory(uniformDevMemory_);
	vk::Viewport viewport;
	viewport.x = viewport.y = 0;
	viewport.width = size.width();
	viewport.height = size.height();
	viewport.minDepth = 0;
	viewport.maxDepth = 1;
	cmdBuffer.setViewport(0, viewport);

	vk::Rect2D scissor;
	scissor.offset.x = scissor.offset.y = 0;
	scissor.extent.width = viewport.width;
	scissor.extent.height = viewport.height;

	cmdBuffer.setScissor(0, scissor);

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipline_);

	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, piplineLayout_, 0, 1, &descSet_[mWindow->currentFrame()], 0, nullptr);

	cmdBuffer.bindVertexBuffers(0, vertexBuffer_, { 0 });

	cmdBuffer.draw(3, 1, 0, 0);

	cmdBuffer.endRenderPass();

	mWindow->frameReady();
	mWindow->requestUpdate();
}

vk::ShaderModule TextureRenderer::loadShader(const QString& name)
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
	vk::Device device = mWindow->device();
	return device.createShaderModule(shaderInfo);
}