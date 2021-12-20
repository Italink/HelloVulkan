#ifndef VulkanWindow_h__
#define VulkanWindow_h__

#include <QVulkanWindowRenderer>
#include <vulkan\vulkan.hpp>
#include "ParticlesSystem.h"

class VulkanRenderer : public QVulkanWindowRenderer {
public:
	VulkanRenderer(QVulkanWindow* window);
	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;
	void startNextFrame() override;
public:
	vk::ShaderModule loadShader(const QString& name);
public:
	QVulkanWindow* window_ = nullptr;
	vk::PipelineCache piplineCache_;
	ParticleSystem particleSystem_;
};

class VulkanWindow : public QVulkanWindow {
public:
	QVulkanWindowRenderer* createRenderer() override { return new VulkanRenderer(this); }
};

#endif // VulkanWindow_h__
