#ifndef BloomRenderer_h__
#define BloomRenderer_h__

#include "QVkWindow.h"
#include "TextureRenderer.h"

class BloomRenderer :public QVkRenderer {
public:
	BloomRenderer();
	void setBlurSize(int size);
	void setBlurStrength(float strength);
	void setBlurScale(float scale);

	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;
	void startNextFrame() override;
private:
	struct FrameBuffer {
		vk::Framebuffer framebuffer;
		vk::Image image;
		vk::DeviceMemory imageMemory;
		vk::ImageView imageView;
	}frameBuffer_[2];

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

	TextureRenderer textureRenderer;
};

#endif // BloomRenderer_h__
