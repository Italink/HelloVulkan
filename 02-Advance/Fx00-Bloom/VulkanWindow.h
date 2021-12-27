#ifndef VulkanWindow_h__
#define VulkanWindow_h__

#include <QVulkanWindowRenderer>
#include "TrianglePipline.h"
#include "BloomPipline.h"
#include "FullScreenPipline.h"

class TriangleRenderer : public QVulkanWindowRenderer {
public:
	TriangleRenderer(QVulkanWindow* window);
	void initResources() override;
	void startNextFrame() override;
	void releaseResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
private:
	QVulkanWindow* window_ = nullptr;
	TrianglePipline trianglePipline_;
	BloomPipline bloomPipline_;
	FullScreenPipline fullScreenPipline_;
};

class VulkanWindow : public QVulkanWindow {
public:
	QVulkanWindowRenderer* createRenderer() override { return new TriangleRenderer(this); }
};

#endif // VulkanWindow_h__
