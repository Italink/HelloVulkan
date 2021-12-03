#include "VulkanWindow.h"
#include <QFile>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE



static float vertexData[] = { // Y up, front = CCW
	 0.0f,   0.5f,   1.0f, 0.0f, 0.0f,
	-0.5f,  -0.5f,   0.0f, 1.0f, 0.0f,
	 0.5f,  -0.5f,   0.0f, 0.0f, 1.0f
};

TriangleRenderer::TriangleRenderer(QVulkanWindow* window)
	:window_(window)
{

}

void TriangleRenderer::initResources()
{
	vk::Device device = window_->device();
	vk::PhysicalDevice physicalDevice = window_->physicalDevice();
	std::vector <vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
	for (int i = 0; i < queueFamilyProperties.size(); i++) {
		if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute) {
			computeQueue_ = device.getQueue(window_->graphicsQueueFamilyIndex(),i);
			break;
		}
	}

	vk::BufferCreateInfo bufferInfo;
	bufferInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;
	bufferInfo.size = BUFFER_SIZE;

	inputBuffer_ = device.createBuffer(bufferInfo);
	outputBuffer_ = device.createBuffer(bufferInfo);

	vk::MemoryAllocateInfo memoryAllocInfo(BUFFER_SIZE * 2,window_->hostVisibleMemoryIndex());
	bufMemory_ = device.allocateMemory(memoryAllocInfo);

	device.bindBufferMemory(inputBuffer_,bufMemory_,0);
	device.bindBufferMemory(outputBuffer_,bufMemory_,BUFFER_SIZE);

	vk::ShaderModule computeShaderModule = loadShader("");

	vk::DescriptorPoolSize descPoolSize;
	descPoolSize.type = vk::DescriptorType::eStorageBuffer;
	descPoolSize.descriptorCount = 2;
	vk::DescriptorPoolCreateInfo descPoolInfo;
	descPoolInfo.maxSets = 1;
	descPoolInfo.poolSizeCount = 1;
	descPoolInfo.pPoolSizes = &descPoolSize;
	descPool_ = device.createDescriptorPool(descPoolInfo);

	vk::DescriptorSetLayoutBinding descSetLayoutBingding[2];
	descSetLayoutBingding[0].binding = 0;
	descSetLayoutBingding[0].descriptorType = vk::DescriptorType::eUniformBuffer;
	descSetLayoutBingding[0].descriptorCount = 1;
	descSetLayoutBingding[0].stageFlags = vk::ShaderStageFlagBits::eVertex;
	descSetLayoutBingding[0].pImmutableSamplers = nullptr;
	descSetLayoutBingding[1] = descSetLayoutBingding[0];
	descSetLayoutBingding[1].binding = 1;

	vk::DescriptorSetLayoutCreateInfo descSetLayoutInfo;
	descSetLayoutInfo.bindingCount = 2;
	descSetLayoutInfo.pBindings = descSetLayoutBingding;
	descSetLayout_ = device.createDescriptorSetLayout(descSetLayoutInfo);

	vk::DescriptorSetAllocateInfo descSetAllocInfo;
	descSetAllocInfo.descriptorPool = descPool_;
	descSetAllocInfo.descriptorSetCount = 1;
	descSetAllocInfo.pSetLayouts = &descSetLayout_;
	descSet_ = device.allocateDescriptorSets(descSetAllocInfo).front();

	descBufferInfo_[0].buffer = inputBuffer_;
	descBufferInfo_[0].range = VK_WHOLE_SIZE;

	descBufferInfo_[1].buffer = outputBuffer_;
	descBufferInfo_[1].range = VK_WHOLE_SIZE;

	vk::WriteDescriptorSet writeDescSet[2];
	writeDescSet[0].dstSet = descSet_;
	writeDescSet[0].dstBinding = 0;
	writeDescSet[0].dstArrayElement = 0;
	writeDescSet[0].descriptorCount = 1;
	writeDescSet[0].descriptorType = descSetLayoutBingding[0].descriptorType;
	writeDescSet[0].pBufferInfo = &descBufferInfo_[0];
	writeDescSet[1] = writeDescSet[0];
	writeDescSet[1].dstBinding = 1;

	device.updateDescriptorSets(2, writeDescSet ,0,nullptr);

	vk::PipelineLayoutCreateInfo piplineLayoutInfo;
	piplineLayoutInfo.setLayoutCount = 1;
	piplineLayoutInfo.pSetLayouts = &descSetLayout_;
	piplineLayout_ = device.createPipelineLayout(piplineLayoutInfo);

	piplineCache_ = device.createPipelineCache(vk::PipelineCacheCreateInfo());

	vk::ComputePipelineCreateInfo computePiplineInfo;
	vk::PipelineShaderStageCreateInfo computeStageInfo;
	computeStageInfo.stage = vk::ShaderStageFlagBits::eCompute ;
	computeStageInfo.module = computeShaderModule;
	computeStageInfo.pName = "main";
	computePiplineInfo.stage = computeStageInfo;
	computePiplineInfo.layout = piplineLayout_;
	computePipline_ = device.createComputePipeline(piplineCache_,computePiplineInfo).value;
}

void TriangleRenderer::initSwapChainResources(){

}

void TriangleRenderer::releaseSwapChainResources()
{

}

void TriangleRenderer::releaseResources(){
	vk::Device device = window_->device();
}

void TriangleRenderer::startNextFrame(){
	vk::Device device = window_->device();
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