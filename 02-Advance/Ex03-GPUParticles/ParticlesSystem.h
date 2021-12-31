#ifndef ParticlesSystem_h__
#define ParticlesSystem_h__

#include "QVulkanWindow"
#include "vulkan/vulkan.hpp"
#include "QFpsCamera.h"

class ParticlesRenderer;

class ParticleSystem {
public:
	ParticleSystem(ParticlesRenderer* renderer);
	void initResource();
	void create(int num);
	void run();
	void swap();
	void render();
	void releaseResource();
private:
	ParticlesRenderer* renderer_;
	vk::Buffer buffer_;
	vk::DeviceMemory bufMemory_;

	vk::DescriptorSetLayout descSetLayout_;
	vk::DescriptorPool descPool_;
	std::array<vk::DescriptorSet, 2> descSet_;
	std::array<vk::DescriptorBufferInfo, 2> descBufInfo_;

	vk::PipelineLayout runnerPiplineLayout_;
	vk::Pipeline runnerPipline_;

	vk::PipelineLayout renderPiplineLayout_;
	vk::Pipeline renderPipline_;
	vk::PushConstantRange pushConstant_;

	int currentNumOfParticles = 0;

	QFpsCamera camera;
};

#endif // ParticlesSystem_h__
