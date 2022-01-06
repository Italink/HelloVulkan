#ifndef TriangleRenderer_h__
#define TriangleRenderer_h__

#include <QVulkanWindowRenderer>
#include <vulkan\vulkan.hpp>
#include "QFpsCamera.h"
#include "QVkWindow.h"

class QVkCameraWindow :public QVkWindow {
public:
	QVkCameraWindow() {
		camera_.setup(this);
	}
	QFpsCamera camera_;
};

class TriangleRenderer : public QVkRenderer {
public:
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
};

#endif // TriangleRenderer_h__