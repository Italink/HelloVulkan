#ifndef SkeletonMeshRenderer_h__
#define SkeletonMeshRenderer_h__

#include <QVulkanWindow>
#include <vulkan\vulkan.hpp>
#include "QFpsCamera.h"
#include "SkeletonMesh.h"

class SkeletonMeshRenderer : public QVulkanWindowRenderer {
	friend class SkeletonMeshNode;
public:
	SkeletonMeshRenderer(QVulkanWindow* window);
protected:
	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;
	void startNextFrame() override;
private:
	QVulkanWindow* window_ = nullptr;
	vk::Device device_;
	SkeletonMesh staticMesh_;
	QFpsCamera camera_;
};

#endif // SkeletonMeshRenderer_h__
