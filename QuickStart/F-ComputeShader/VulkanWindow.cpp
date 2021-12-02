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
	
}

void TriangleRenderer::initSwapChainResources()
{

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