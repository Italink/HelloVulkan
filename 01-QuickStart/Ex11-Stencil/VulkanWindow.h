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

	vk::Buffer vertexBuffer_;
	vk::DeviceMemory vertexDevMemory_;

	vk::DescriptorPool descPool_;
	vk::DescriptorSetLayout descSetLayout_;
	vk::DescriptorSet descSet_[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

	vk::PushConstantRange pushConstant_;

	vk::PipelineCache piplineCache_;
	vk::PipelineLayout piplineLayout_;
	vk::Pipeline pipline_;
	vk::Pipeline outlinePipline_;
};

class VulkanWindow: public QVulkanWindow{
public:
	QVulkanWindowRenderer* createRenderer() override{ return new TriangleRenderer(this); }
};

#endif // VulkanWindow_h__

