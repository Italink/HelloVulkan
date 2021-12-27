#ifndef VulkanWindow_h__
#define VulkanWindow_h__

#include <QVulkanWindowRenderer>
#include <vulkan\vulkan.hpp>
class TriangleRenderer : public QVulkanWindowRenderer {
public:
	TriangleRenderer(QVulkanWindow* window);
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

	vk::Buffer indexBuffer_;
	vk::DeviceMemory indexDevMemory_;

	vk::PipelineCache piplineCache_;
	vk::PipelineLayout piplineLayout_;
	vk::Pipeline pipline_;
};

class VulkanWindow : public QVulkanWindow {
public:
	QVulkanWindowRenderer* createRenderer() override { return new TriangleRenderer(this); }
};

#endif // VulkanWindow_h__
