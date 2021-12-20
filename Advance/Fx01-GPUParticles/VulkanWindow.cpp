#include "VulkanWindow.h"
#include <QFile>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

VulkanRenderer::VulkanRenderer(QVulkanWindow* window)
	: particleSystem_(this)
	, window_(window)
{
}

void VulkanRenderer::initResources()
{
	vk::Device device = window_->device();
	piplineCache_ = device.createPipelineCache(vk::PipelineCacheCreateInfo());
	particleSystem_.initResource();
}

void VulkanRenderer::initSwapChainResources() {
}

void VulkanRenderer::releaseSwapChainResources()
{
}

void VulkanRenderer::releaseResources() {
	particleSystem_.releaseResource();
	vk::Device device = window_->device();
	device.destroyPipelineCache(piplineCache_);
}

void VulkanRenderer::startNextFrame() {
	particleSystem_.create(100);
	particleSystem_.run();
	particleSystem_.render();
	window_->frameReady();
	window_->requestUpdate();
}

vk::ShaderModule VulkanRenderer::loadShader(const QString& name)
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