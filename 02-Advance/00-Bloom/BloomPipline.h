#ifndef BloomPipline_h__
#define BloomPipline_h__

#include <QVulkanWindow>
#include <vulkan\vulkan.hpp>

class BloomPipline {
public:
	BloomPipline(QVulkanWindow* window);

	void setBlurSize(int size);
	void setBlurStrength(float strength);
	void setBlurScale(float scale);

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
	vk::RenderPass renderPass_;
	vk::DescriptorPool descPool_;
	vk::DescriptorSetLayout descSetLayout_;
	vk::DescriptorSet descSet_[2];
	vk::PipelineCache piplineCache_;
	vk::PipelineLayout piplineLayout_;
	vk::Pipeline hBlurPipline_;
	vk::Pipeline vBlurPipline_;

	vk::PushConstantRange pushConstant_;

	struct BlurParams {
		float scale = 1.0;
		float strength = 2.0;
		int size = 5;
		float weight[40] = { 0.227027f,0.1945946f,0.1216216f,0.054054f , 0.016216f };
	}blurParams_;

private:
	QVulkanWindow* window_ = nullptr;
};

#endif // BloomPipline_h__
