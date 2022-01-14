#ifndef InstancingRenderer_h__
#define InstancingRenderer_h__

#include <QVulkanWindowRenderer>
#include <vulkan\vulkan.hpp>

class InstancingRenderer : public QVulkanWindowRenderer {
public:
	InstancingRenderer(QVulkanWindow* window);
	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;
	void startNextFrame() override;
private:
	QVulkanWindow* window_ = nullptr;

	vk::Buffer vertexBuffer_;
	vk::DeviceMemory vertexDevMemory_;

	vk::Buffer instanceBuffer_;
	vk::DeviceMemory instanceDevMemory_;

	vk::DescriptorPool descPool_;
	vk::DescriptorSetLayout descSetLayout_;
	std::array<vk::DescriptorSet, QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT> descSet_;

	vk::PipelineCache piplineCache_;
	vk::PipelineLayout piplineLayout_;
	vk::Pipeline pipline_;
};

#endif // InstancingRenderer_h__
