#ifndef BloomRenderer_h__
#define BloomRenderer_h__

#include <vulkan\vulkan.hpp>

class BloomRenderer {
public:
	BloomRenderer();
	void setBlurSize(int size);
	void setBlurStrength(float strength);
	void setBlurScale(float scale);

	vk::ImageView getOutputImageView();

	void init(vk::Device device);
	void render(vk::Image image, int width, int height, vk::CommandBuffer cmdBuffer);
	void destroy();
	void updateFrameBuffer(int width, int height, uint32_t memoryIndex);
private:
	void destroyFrameBuffer();
	bool isInitialized();
private:
	vk::Device device;

	struct FrameBuffer {
		vk::Framebuffer framebuffer;
		vk::Image image;
		vk::DeviceMemory imageMemory;
		vk::ImageView imageView;
	}frameBuffer_[2];
	bool frameBufferFormathasInit = false;

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
};

#endif // BloomRenderer_h__
