#ifndef QImGUIRenderer_h__
#define QImGUIRenderer_h__

#include <QVulkanWindowRenderer>
#include <vulkan\vulkan.hpp>
#include <imgui.h>
#include <implot.h>

class QImGUIRenderer :public QObject, public QVulkanWindowRenderer {
public:
	QImGUIRenderer(QVulkanWindow* window);
	~QImGUIRenderer();
protected:
	bool eventFilter(QObject* watched, QEvent* event) override;
	virtual void initResources() override;
	virtual void initSwapChainResources() override;
	virtual void releaseSwapChainResources() override;
	virtual void releaseResources() override;
	virtual void startNextFrame() override;
	void frameReady();
	void requestUpdate();
	void submitImGui();
protected:
	QVulkanWindow* window_ = nullptr;
private:
	ImGuiContext* imGuiContext = nullptr;
	ImPlotContext* imPlotContext = nullptr;
	double       time_ = 0.0f;
	bool         mousePressed_[3] = { false, false, false };
	float        mouseWheel_ = 0;
	float        mouseWheelH_ = 0;

	vk::Buffer vertexBuffer_;
	vk::DeviceMemory vertexDevMemory_;

	vk::Buffer indexBuffer_;
	vk::DeviceMemory indexDevMemory_;

	vk::PushConstantRange matMVP_;

	vk::DescriptorPool descPool_;
	vk::DescriptorSetLayout descSetLayout_;
	vk::DescriptorSet descSet_;

	vk::PipelineCache piplineCache_;
	vk::PipelineLayout piplineLayout_;
	vk::Pipeline pipline_;

	vk::Image fontTexture_;
	vk::DeviceMemory fontTexMemory_;
	vk::ImageView fontTexView_;
	vk::Sampler sampler_;
};

#endif // QImGUIRenderer_h__
