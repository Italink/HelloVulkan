#ifndef FullScreenPipline_h__
#define FullScreenPipline_h__

#include <QVulkanWindow>
#include <vulkan\vulkan.hpp>

class FullScreenPipline {
public:
	FullScreenPipline(QVulkanWindow* window);
	void init();
	void render();
	void destroy();
	void updateImage(vk::ImageView image, vk::Sampler sampler);
protected:
	vk::DescriptorPool descPool_;
	vk::DescriptorSetLayout descSetLayout_;
	vk::DescriptorSet descSet_;
	vk::PipelineCache piplineCache_;
	vk::PipelineLayout piplineLayout_;
	vk::Pipeline pipline_;
private:
	QVulkanWindow* window_ = nullptr;
};

#endif // FullScreenPipline_h__
