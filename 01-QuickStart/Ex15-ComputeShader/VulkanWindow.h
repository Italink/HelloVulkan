#ifndef VulkanWindow_h__
#define VulkanWindow_h__

#include <QVulkanWindowRenderer>
#include <vulkan\vulkan.hpp>
class VulkanRenderer: public QVulkanWindowRenderer {
public:
	VulkanRenderer(QVulkanWindow * window);
	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;
	void startNextFrame() override;
private:
	vk::ShaderModule loadShader(const QString& name);
private:

	QVulkanWindow* window_ = nullptr;

	vk::Buffer buffer_;
	vk::DeviceMemory bufMemory_;

	vk::DescriptorPool descPool_;
	vk::DescriptorSetLayout descSetLayout_;
	vk::DescriptorSet descSet_;
	vk::DescriptorBufferInfo descBufferInfo_;
	
	vk::PipelineLayout piplineLayout_;
	vk::Pipeline computePipline_;
	vk::PipelineCache piplineCache_;

	enum DisplayId : int32_t{
		WorkGroupId = 0,
		LocalId,
		GlobalId,
		SharedId,
	}displayId_ = WorkGroupId;
};

class VulkanWindow: public QVulkanWindow{
public:
	QVulkanWindowRenderer* createRenderer() override{ return new VulkanRenderer(this); }
};

#endif // VulkanWindow_h__

