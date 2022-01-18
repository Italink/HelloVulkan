#ifndef TextureRenderer_h__
#define TextureRenderer_h__

#include "QVkWindow.h"

class TextureRenderer :public QVkRenderer {
public:
	TextureRenderer();

	void initResources() override;
	void releaseResources() override;
	void startNextFrame() override;
	void updateImage(vk::ImageView image);

protected:
	vk::Sampler sampler_;

	vk::DescriptorPool descPool_;
	vk::DescriptorSetLayout descSetLayout_;
	vk::DescriptorSet descSet_;

	vk::PipelineCache piplineCache_;
	vk::PipelineLayout piplineLayout_;
	vk::Pipeline pipline_;
};

#endif // TextureRenderer_h__
