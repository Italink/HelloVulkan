#include "BufferComputer.h"
#include "Shaders/buffer_comp.inl"

const float data[16] = { 0 };

BufferComputer::BufferComputer(QVulkanWindow* window)
	:window_(window)
{
}

void BufferComputer::initResources()
{
	vk::Device device = window_->device();

	vk::BufferCreateInfo bufferInfo;
	bufferInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;
	bufferInfo.size = sizeof(data);

	buffer_ = device.createBuffer(bufferInfo);

	vk::MemoryRequirements memReq = device.getBufferMemoryRequirements(buffer_);
	vk::MemoryAllocateInfo memoryAllocInfo(memReq.size, window_->hostVisibleMemoryIndex());
	bufMemory_ = device.allocateMemory(memoryAllocInfo);
	device.bindBufferMemory(buffer_, bufMemory_, 0);

	uint8_t* memPtr = (uint8_t*)device.mapMemory(bufMemory_, 0, bufferInfo.size, {});
	memcpy(memPtr, data, bufferInfo.size);
	device.unmapMemory(bufMemory_);

	vk::DescriptorSetLayoutBinding descSetLayoutBingding;
	descSetLayoutBingding.binding = 0;
	descSetLayoutBingding.descriptorType = vk::DescriptorType::eStorageBuffer;
	descSetLayoutBingding.descriptorCount = 1;
	descSetLayoutBingding.stageFlags = vk::ShaderStageFlagBits::eCompute;
	descSetLayoutBingding.pImmutableSamplers = nullptr;

	vk::DescriptorSetLayoutCreateInfo descSetLayoutInfo;
	descSetLayoutInfo.bindingCount = 1;
	descSetLayoutInfo.pBindings = &descSetLayoutBingding;
	descSetLayout_ = device.createDescriptorSetLayout(descSetLayoutInfo);

	vk::DescriptorPoolSize descPoolSize;
	descPoolSize.type = vk::DescriptorType::eStorageBuffer;
	descPoolSize.descriptorCount = 1;

	vk::DescriptorPoolCreateInfo descPoolInfo;
	descPoolInfo.maxSets = 1;
	descPoolInfo.poolSizeCount = 1;
	descPoolInfo.pPoolSizes = &descPoolSize;
	descPool_ = device.createDescriptorPool(descPoolInfo);

	vk::DescriptorSetAllocateInfo descSetAllocInfo;
	descSetAllocInfo.descriptorPool = descPool_;
	descSetAllocInfo.descriptorSetCount = 1;
	descSetAllocInfo.pSetLayouts = &descSetLayout_;
	descSet_ = device.allocateDescriptorSets(descSetAllocInfo).front();

	descBufferInfo_.buffer = buffer_;
	descBufferInfo_.range = bufferInfo.size;

	vk::WriteDescriptorSet writeDescSet;
	writeDescSet.dstSet = descSet_;
	writeDescSet.dstBinding = 0;
	writeDescSet.descriptorCount = 1;
	writeDescSet.descriptorType = descSetLayoutBingding.descriptorType;
	writeDescSet.pBufferInfo = &descBufferInfo_;
	device.updateDescriptorSets(1, &writeDescSet, 0, nullptr);

	vk::PipelineLayoutCreateInfo piplineLayoutInfo;
	piplineLayoutInfo.setLayoutCount = 1;
	piplineLayoutInfo.pSetLayouts = &descSetLayout_;
	piplineLayout_ = device.createPipelineLayout(piplineLayoutInfo);

	piplineCache_ = device.createPipelineCache(vk::PipelineCacheCreateInfo());

	vk::ShaderModuleCreateInfo shaderInfo;
	shaderInfo.codeSize = sizeof(buffer_comp);
	shaderInfo.pCode = buffer_comp;
	vk::ShaderModule computeShaderModule = device.createShaderModule(shaderInfo);

	vk::SpecializationMapEntry specMapEntry;
	specMapEntry.constantID = 0;
	specMapEntry.offset = 0;
	specMapEntry.size = sizeof(displayId_);

	vk::SpecializationInfo specInfo;
	specInfo.dataSize = sizeof(displayId_);
	specInfo.mapEntryCount = 1;
	specInfo.pMapEntries = &specMapEntry;
	specInfo.pData = &displayId_;

	vk::PipelineShaderStageCreateInfo computeStageInfo;
	computeStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
	computeStageInfo.module = computeShaderModule;
	computeStageInfo.pName = "main";
	computeStageInfo.pSpecializationInfo = &specInfo;

	vk::ComputePipelineCreateInfo computePiplineInfo;
	computePiplineInfo.stage = computeStageInfo;
	computePiplineInfo.layout = piplineLayout_;
	computePipline_ = device.createComputePipeline(piplineCache_, computePiplineInfo).value;

	vk::CommandBufferAllocateInfo cmdBufferAlllocInfo;
	cmdBufferAlllocInfo.level = vk::CommandBufferLevel::ePrimary;
	cmdBufferAlllocInfo.commandPool = window_->graphicsCommandPool();
	cmdBufferAlllocInfo.commandBufferCount = 1;
	vk::CommandBuffer cmdBuffer = device.allocateCommandBuffers(cmdBufferAlllocInfo).front();

	vk::CommandBufferBeginInfo cmdBeginInfo;
	cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	cmdBuffer.begin(cmdBeginInfo);
	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipline_);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, piplineLayout_, 0, 1, &descSet_, 0, nullptr);
	cmdBuffer.dispatch(2, 2, 1);
	cmdBuffer.end();

	vk::Queue graphicsQueue = window_->graphicsQueue();
	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	graphicsQueue.submit(submitInfo);
	graphicsQueue.waitIdle();
	device.freeCommandBuffers(window_->graphicsCommandPool(), cmdBuffer);

	float readBack[16];
	memPtr = (uint8_t*)device.mapMemory(bufMemory_, 0, bufferInfo.size, {});
	memcpy((uint8_t*)readBack, memPtr, bufferInfo.size);
	device.unmapMemory(bufMemory_);

	qDebug() << readBack[0] << "\t" << readBack[1] << "\t" << readBack[2] << "\t" << readBack[3] << "\n"
		<< readBack[4] << "\t" << readBack[5] << "\t" << readBack[6] << "\t" << readBack[7] << "\n"
		<< readBack[8] << "\t" << readBack[9] << "\t" << readBack[10] << "\t" << readBack[11] << "\n"
		<< readBack[12] << "\t" << readBack[13] << "\t" << readBack[14] << "\t" << readBack[15] << "\n";

	device.destroyShaderModule(computeShaderModule);
}

void BufferComputer::initSwapChainResources() {
}

void BufferComputer::releaseSwapChainResources()
{
}

void BufferComputer::releaseResources() {
	vk::Device device = window_->device();
	device.destroyBuffer(buffer_);
	device.freeMemory(bufMemory_);
	device.destroyDescriptorPool(descPool_);
	device.destroyDescriptorSetLayout(descSetLayout_);
	device.destroyPipeline(computePipline_);
	device.destroyPipelineLayout(piplineLayout_);
	device.destroyPipelineCache(piplineCache_);
}

void BufferComputer::startNextFrame() {
}