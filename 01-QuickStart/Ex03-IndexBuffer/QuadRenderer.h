#ifndef QuadRenderer_h__
#define QuadRenderer_h__

#include <QVulkanWindowRenderer>
#include <vulkan\vulkan.hpp>
class QuadRenderer : public QVulkanWindowRenderer {
public:
	QuadRenderer(QVulkanWindow* window);
	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;
	void startNextFrame() override;
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

#endif // QuadRenderer_h__
