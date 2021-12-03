#ifndef VulkanWindow_h__
#define VulkanWindow_h__

#include <QVulkanWindowRenderer>
#include <vulkan\vulkan.hpp>
class TriangleRenderer: public QVulkanWindowRenderer {
public:
	TriangleRenderer(QVulkanWindow * window);
	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;
	void startNextFrame() override;
private:
	vk::ShaderModule loadShader(const QString& name);
private:
	QVulkanWindow* window_ = nullptr;
	vk::Queue computeQueue_;

	const int BUFFER_SIZE = 16384;
	vk::Buffer inputBuffer_;
	vk::Buffer outputBuffer_;
	vk::DeviceMemory bufMemory_;

	vk::DescriptorPool descPool_;
	vk::DescriptorSetLayout descSetLayout_;
	vk::DescriptorSet descSet_;
	vk::DescriptorBufferInfo descBufferInfo_[2];
	
	vk::PipelineLayout piplineLayout_;
	vk::Pipeline computePipline_;
	vk::PipelineCache piplineCache_;
};

class VulkanWindow: public QVulkanWindow{
public:
	QVulkanWindowRenderer* createRenderer() override{ return new TriangleRenderer(this); }
};

#endif // VulkanWindow_h__

