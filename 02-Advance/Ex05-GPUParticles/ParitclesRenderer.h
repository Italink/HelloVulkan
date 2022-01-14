#ifndef ParitclesRenderer_h__
#define ParitclesRenderer_h__

#include <QVulkanWindowRenderer>
#include <vulkan\vulkan.hpp>
#include "ParticlesSystem.h"

class ParticlesRenderer : public QVulkanWindowRenderer {
public:
	ParticlesRenderer(QVulkanWindow* window);
	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;
	void startNextFrame() override;
public:
	QVulkanWindow* window_ = nullptr;
	vk::PipelineCache piplineCache_;
	ParticleSystem particleSystem_;
};

#endif // ParitclesRenderer_h__