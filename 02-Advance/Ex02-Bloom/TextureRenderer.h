#ifndef TextureRenderer_h__
#define TextureRenderer_h__

#include <vulkan\vulkan.hpp>

class TextureRenderer {
public:
	TextureRenderer();
	void init(vk::Device device, vk::SampleCountFlagBits sampleCount, vk::RenderPass renderPass);
	void render(vk::CommandBuffer cmdBuffer);
	void destroy();
	void updateImage(vk::ImageView image);
	void updateRect(int x, int y, int width, int height);
protected:
	vk::Sampler sampler_;

	vk::DescriptorPool descPool_;
	vk::DescriptorSetLayout descSetLayout_;
	vk::DescriptorSet descSet_;

	vk::PipelineCache piplineCache_;
	vk::PipelineLayout piplineLayout_;
	vk::Pipeline pipline_;
private:
	vk::Device device;
};

#endif // TextureRenderer_h__
