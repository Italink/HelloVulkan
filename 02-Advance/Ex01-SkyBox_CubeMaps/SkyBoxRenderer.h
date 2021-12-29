#ifndef SkyBoxRenderer_h__
#define SkyBoxRenderer_h__

#include <QVulkanWindowRenderer>
#include <vulkan\vulkan.hpp>
#include "QFPSCamera.h"

class SkyBoxRenderer : public QVulkanWindowRenderer {
public:
	SkyBoxRenderer(QVulkanWindow* window);
	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;
	void startNextFrame() override;
private:
	QVulkanWindow* window_ = nullptr;

	vk::Buffer vertexBuffer_;
	vk::DeviceMemory vertexDevMemory_;

	vk::DescriptorPool descPool_;
	vk::DescriptorSetLayout descSetLayout_;
	vk::DescriptorSet descSet_;

	vk::PipelineCache piplineCache_;
	vk::PipelineLayout piplineLayout_;
	vk::Pipeline pipline_;

	vk::Sampler sampler_;
	vk::Image image_;
	vk::DeviceMemory imageDevMemory_;
	vk::ImageView imageView_;

	QFPSCamera fpsCamera_;
};

#endif
