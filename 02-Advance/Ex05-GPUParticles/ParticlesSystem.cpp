#include "ParticlesSystem.h"
#include "ParitclesRenderer.h"
#include "QDateTime"

#include "Shaders/particle_renderer_vert.inl"
#include "Shaders/particle_renderer_frag.inl"
#include "Shaders/particle_runner_comp.inl"

#define LOCAL_SIZE 32

#define PARTICLE_MAX_SIZE 100000

struct Particle {
	QVector3D position;
	float life;
	QVector3D velocity;
	int32_t padding[1];
};

struct ParticlesBuffer {
	int counter;
	int32_t padding[3];
	Particle particles[PARTICLE_MAX_SIZE];
};

static inline vk::DeviceSize aligned(vk::DeviceSize v, vk::DeviceSize byteAlign) {
	return (v + byteAlign - 1) & ~(byteAlign - 1);
}

ParticleSystem::ParticleSystem(ParticlesRenderer* window) : renderer_(window)
{
	camera.setup(renderer_->window_);
}

void ParticleSystem::initResource()
{
	vk::Device device = renderer_->window_->device();
	vk::BufferCreateInfo bufInfo;
	vk::DeviceSize alignBufferSize = aligned(sizeof(ParticlesBuffer), renderer_->window_->physicalDeviceProperties()->limits.minStorageBufferOffsetAlignment);
	bufInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer;
	bufInfo.sharingMode = vk::SharingMode::eExclusive;
	bufInfo.size = 2 * alignBufferSize;
	buffer_ = device.createBuffer(bufInfo);

	vk::MemoryRequirements memReq = device.getBufferMemoryRequirements(buffer_);
	vk::MemoryAllocateInfo memAllocInfo(memReq.size, renderer_->window_->hostVisibleMemoryIndex());
	bufMemory_ = device.allocateMemory(memAllocInfo);
	device.bindBufferMemory(buffer_, bufMemory_, 0);

	vk::DescriptorSetLayoutBinding descSetLayoutBinding[2];
	descSetLayoutBinding[0].binding = 0;
	descSetLayoutBinding[0].descriptorCount = 1;
	descSetLayoutBinding[0].descriptorType = vk::DescriptorType::eStorageBuffer;
	descSetLayoutBinding[0].stageFlags = vk::ShaderStageFlagBits::eCompute;
	descSetLayoutBinding[1] = descSetLayoutBinding[0];
	descSetLayoutBinding[1].binding = 1;

	vk::DescriptorSetLayoutCreateInfo descSetLayoutInfo;
	descSetLayoutInfo.bindingCount = 2;
	descSetLayoutInfo.pBindings = descSetLayoutBinding;
	descSetLayout_ = device.createDescriptorSetLayout(descSetLayoutInfo);

	vk::DescriptorPoolSize descPoolSize;
	descPoolSize.descriptorCount = 4;
	descPoolSize.type = vk::DescriptorType::eStorageBuffer;

	vk::DescriptorPoolCreateInfo descPoolInfo;
	descPoolInfo.maxSets = 2;
	descPoolInfo.poolSizeCount = 1;
	descPoolInfo.pPoolSizes = &descPoolSize;
	descPool_ = device.createDescriptorPool(descPoolInfo);

	vk::DescriptorSetAllocateInfo descSetAllocInfo;
	descSetAllocInfo.descriptorPool = descPool_;
	descSetAllocInfo.descriptorSetCount = 1;
	descSetAllocInfo.pSetLayouts = &descSetLayout_;
	descSet_[0] = device.allocateDescriptorSets(descSetAllocInfo).front();
	descSet_[1] = device.allocateDescriptorSets(descSetAllocInfo).front();

	descBufInfo_[0].buffer = buffer_;
	descBufInfo_[0].offset = 0;
	descBufInfo_[0].range = alignBufferSize;

	descBufInfo_[1].buffer = buffer_;
	descBufInfo_[1].offset = alignBufferSize;
	descBufInfo_[1].range = alignBufferSize;

	for (int i = 0; i < std::size(descSetLayoutBinding); i++) {
		vk::WriteDescriptorSet descSetWriter;
		descSetWriter.descriptorCount = 1;
		descSetWriter.descriptorType = vk::DescriptorType::eStorageBuffer;
		descSetWriter.dstSet = descSet_[0];
		descSetWriter.dstBinding = descSetLayoutBinding[i].binding;
		descSetWriter.descriptorType = descSetLayoutBinding[i].descriptorType;
		descSetWriter.pBufferInfo = &descBufInfo_[i];
		device.updateDescriptorSets(1, &descSetWriter, 0, nullptr);

		descSetWriter.dstSet = descSet_[1];
		descSetWriter.pBufferInfo = &descBufInfo_[1 - i];
		device.updateDescriptorSets(1, &descSetWriter, 0, nullptr);
	}

	vk::ShaderModuleCreateInfo shaderInfo;
	shaderInfo.codeSize = sizeof(particle_runner_comp);
	shaderInfo.pCode = particle_runner_comp;
	vk::ShaderModule computeShaderModule = device.createShaderModule(shaderInfo);

	vk::PipelineLayoutCreateInfo piplineLayoutInfo;
	piplineLayoutInfo.setLayoutCount = 1;
	piplineLayoutInfo.pSetLayouts = &descSetLayout_;
	runnerPiplineLayout_ = device.createPipelineLayout(piplineLayoutInfo);

	vk::PipelineShaderStageCreateInfo computeStageInfo;
	computeStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
	computeStageInfo.module = computeShaderModule;
	computeStageInfo.pName = "main";

	vk::ComputePipelineCreateInfo computePiplineInfo;
	computePiplineInfo.stage = computeStageInfo;
	computePiplineInfo.layout = runnerPiplineLayout_;
	runnerPipline_ = device.createComputePipeline(renderer_->piplineCache_, computePiplineInfo).value;

	device.destroyShaderModule(computeShaderModule);

	vk::GraphicsPipelineCreateInfo piplineInfo;
	piplineInfo.stageCount = 2;

	shaderInfo.codeSize = sizeof(particle_renderer_vert);
	shaderInfo.pCode = particle_renderer_vert;
	vk::ShaderModule vertShader = device.createShaderModule(shaderInfo);

	shaderInfo.codeSize = sizeof(particle_renderer_frag);
	shaderInfo.pCode = particle_renderer_frag;
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
	vertexBindingDesc.stride = sizeof(Particle);
	vertexBindingDesc.inputRate = vk::VertexInputRate::eVertex;

	vk::VertexInputAttributeDescription vertexAttrDesc;
	vertexAttrDesc.binding = 0;
	vertexAttrDesc.location = 0;
	vertexAttrDesc.offset = offsetof(ParticlesBuffer, particles);
	vertexAttrDesc.format = vk::Format::eR32G32B32Sfloat;

	vk::PipelineVertexInputStateCreateInfo vertexInputState({}, 1, &vertexBindingDesc, 1, &vertexAttrDesc);
	piplineInfo.pVertexInputState = &vertexInputState;

	vk::PipelineInputAssemblyStateCreateInfo vertexAssemblyState({}, vk::PrimitiveTopology::ePointList);
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
	MSState.rasterizationSamples = (VULKAN_HPP_NAMESPACE::SampleCountFlagBits)renderer_->window_->sampleCountFlagBits();
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
	pushConstant_.size = sizeof(QMatrix4x4);
	pushConstant_.stageFlags = vk::ShaderStageFlagBits::eVertex;

	vk::PipelineLayoutCreateInfo renderPiplineLayoutInfo;
	renderPiplineLayoutInfo.pushConstantRangeCount = 1;
	renderPiplineLayoutInfo.pPushConstantRanges = &pushConstant_;
	renderPiplineLayout_ = device.createPipelineLayout(renderPiplineLayoutInfo);
	piplineInfo.layout = renderPiplineLayout_;

	piplineInfo.renderPass = renderer_->window_->defaultRenderPass();

	renderPipline_ = device.createGraphicsPipeline(renderer_->piplineCache_, piplineInfo).value;

	device.destroyShaderModule(vertShader);
	device.destroyShaderModule(fragShader);
}

void ParticleSystem::create(int num)
{
	vk::Device device = renderer_->window_->device();
	num = std::clamp(num, 0, PARTICLE_MAX_SIZE - currentNumOfParticles);

	std::vector<Particle> particles(num);
	for (auto& particle : particles) {
		particle.life = 0;
		particle.position = QVector3D(0, 0, 0);
		double randomAngle = rand() % 360 / 180.0 * M_PI;
		particle.velocity = QVector3D(std::cos(randomAngle) / 200, -0.005, std::sin(randomAngle) / 200);
	}

	uint8_t* memPtr = (uint8_t*)device.mapMemory(bufMemory_, descBufInfo_[0].offset + offsetof(ParticlesBuffer, particles) + currentNumOfParticles * sizeof(Particle), num * sizeof(Particle), {});
	memcpy(memPtr, particles.data(), num * sizeof(Particle));
	device.unmapMemory(bufMemory_);

	currentNumOfParticles += num;
	memPtr = (uint8_t*)device.mapMemory(bufMemory_, descBufInfo_[0].offset, sizeof(int), {});
	memcpy(memPtr, &currentNumOfParticles, sizeof(int));
	device.unmapMemory(bufMemory_);
}

void ParticleSystem::run()
{
	vk::Device device = renderer_->window_->device();

	vk::CommandBufferAllocateInfo cmdBufferAlllocInfo;
	cmdBufferAlllocInfo.level = vk::CommandBufferLevel::ePrimary;
	cmdBufferAlllocInfo.commandPool = renderer_->window_->graphicsCommandPool();
	cmdBufferAlllocInfo.commandBufferCount = 1;
	vk::CommandBuffer cmdBuffer = device.allocateCommandBuffers(cmdBufferAlllocInfo).front();

	vk::CommandBufferBeginInfo cmdBeginInfo;
	cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	cmdBuffer.begin(cmdBeginInfo);
	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, runnerPipline_);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, runnerPiplineLayout_, 0, 1, &descSet_[0], 0, nullptr);
	int numSqrt = std::ceil(std::sqrt(currentNumOfParticles) / LOCAL_SIZE);
	cmdBuffer.dispatch(numSqrt, numSqrt, 1);
	cmdBuffer.end();

	vk::Queue graphicsQueue = renderer_->window_->graphicsQueue();
	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	graphicsQueue.submit(submitInfo);
	graphicsQueue.waitIdle();
	device.freeCommandBuffers(renderer_->window_->graphicsCommandPool(), cmdBuffer);
	swap();
}

void ParticleSystem::swap() {
	vk::Device device = renderer_->window_->device();
	uint8_t* memPtr = (uint8_t*)device.mapMemory(bufMemory_, descBufInfo_[1].offset, sizeof(int), {});
	memcpy((uint8_t*)&currentNumOfParticles, memPtr, sizeof(int));
	device.unmapMemory(bufMemory_);

	memPtr = (uint8_t*)device.mapMemory(bufMemory_, descBufInfo_[0].offset, sizeof(int), {});
	int reset = 0;
	memcpy(memPtr, &reset, sizeof(int));
	device.unmapMemory(bufMemory_);

	std::swap(descBufInfo_[0], descBufInfo_[1]);
	std::swap(descSet_[0], descSet_[1]);
}

void ParticleSystem::render()
{
	vk::Device device = renderer_->window_->device();
	vk::CommandBuffer cmdBuffer = renderer_->window_->currentCommandBuffer();
	const QSize size = renderer_->window_->swapChainImageSize();

	vk::ClearValue clearValues[3] = {
		vk::ClearColorValue(std::array<float,4>{0.0f,0.5f,0.9f,1.0f }),
		vk::ClearDepthStencilValue(1.0f,0),
		vk::ClearColorValue(std::array<float,4>{ 0.0f,0.5f,0.9f,1.0f }),
	};

	vk::RenderPassBeginInfo beginInfo;
	beginInfo.renderPass = renderer_->window_->defaultRenderPass();
	beginInfo.framebuffer = renderer_->window_->currentFramebuffer();
	beginInfo.renderArea.extent.width = size.width();
	beginInfo.renderArea.extent.height = size.height();
	beginInfo.clearValueCount = renderer_->window_->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
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

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, renderPipline_);

	cmdBuffer.bindVertexBuffers(0, buffer_, { descBufInfo_[0].offset });

	QMatrix4x4 view;
	view *= camera.getMatrix();
	cmdBuffer.pushConstants<QMatrix4x4>(renderPiplineLayout_, pushConstant_.stageFlags, 0, view);

	cmdBuffer.draw(currentNumOfParticles, 1, 0, 0);
	qDebug() << currentNumOfParticles;
	cmdBuffer.endRenderPass();
}

void ParticleSystem::releaseResource()
{
	vk::Device device = renderer_->window_->device();
	device.destroyBuffer(buffer_);
	device.freeMemory(bufMemory_);
	device.destroyDescriptorSetLayout(descSetLayout_);
	device.destroyDescriptorPool(descPool_);
	device.destroyPipeline(runnerPipline_);
	device.destroyPipelineLayout(runnerPiplineLayout_);
	device.destroyPipeline(renderPipline_);
	device.destroyPipelineLayout(renderPiplineLayout_);
}