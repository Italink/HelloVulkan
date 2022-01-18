#ifndef TriangleRenderer_h__
#define TriangleRenderer_h__

#include <QVulkanWindowRenderer>
#include <vulkan\vulkan.hpp>
#include "QFpsCamera.h"

class QVkCameraWindow;

class TriangleRenderer : public QVulkanWindowRenderer {
public:
	TriangleRenderer(QVkCameraWindow* window);
	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;
	void startNextFrame() override;
private:
	vk::Buffer vertexBuffer_;
	vk::DeviceMemory vertexDevMemory_;
	vk::PipelineCache piplineCache_;
	vk::PipelineLayout piplineLayout_;
	vk::Pipeline pipline_;
	QVkCameraWindow* window_;
};

class QVkCameraWindow :public QVulkanWindow {
public:
	QVkCameraWindow() {
		camera_.setup(this);
	}
	QFpsCamera camera_;
protected:
	QVulkanWindowRenderer* createRenderer() override {
		return new TriangleRenderer(this);
	}
};

#endif // TriangleRenderer_h__