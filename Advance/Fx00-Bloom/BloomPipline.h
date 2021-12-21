#ifndef BloomPipline_h__
#define BloomPipline_h__

#include <QVulkanWindow>
#include <vulkan\vulkan.hpp>

class BloomPipline {
public:
	BloomPipline(QVulkanWindow* window);
	void initResource();
	void render();
	void destroy();
	void destroyFrameBuffer();
	bool isInitialized();
public:
	struct FrameBuffer {
		vk::Framebuffer framebuffer;
		vk::Image image;
		vk::DeviceMemory imageMemory;
		vk::ImageView imageView;
	}frameBuffer_[2];
	void resizeFrameBuffer(int width, int height);

	vk::Sampler sampler_;

	vk::Buffer uniformBuffer_;
	vk::DeviceMemory uniformDevMemory_;
	vk::DescriptorBufferInfo uniformBufferInfo_;
	vk::RenderPass renderPass_;
	vk::DescriptorPool descPool_;
	vk::DescriptorSetLayout descSetLayout_;
	vk::DescriptorSet descSet_[2];
	vk::PipelineCache piplineCache_;
	vk::PipelineLayout piplineLayout_;
	vk::Pipeline hBlurPipline_;
	vk::Pipeline vBlurPipline_;
private:
	QVulkanWindow* window_ = nullptr;
};

#endif // BloomPipline_h__
