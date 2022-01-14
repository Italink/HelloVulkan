#ifndef StaticMeshRenderer_h__
#define StaticMeshRenderer_h__

#include <QVulkanWindow>
#include <vulkan\vulkan.hpp>
#include "QFpsCamera.h"
#include "StaticMesh.h"

class StaticMeshRenderer : public QVulkanWindowRenderer {
	friend class StaticMeshNode;
public:
	StaticMeshRenderer(QVulkanWindow* window);
protected:
	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;
	void startNextFrame() override;
private:
	QVulkanWindow* window_ = nullptr;
	vk::Device device_;
	StaticMesh staticMesh_;
	QFpsCamera camera_;
};

#endif // StaticMeshRenderer_h__
