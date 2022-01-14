#ifndef VulkanWindow_h__
#define VulkanWindow_h__

#include <QVulkanWindow>

#include "TrianglePipline.h"
#include "BloomRenderer.h"
#include "TextureRenderer.h"

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
	BloomRenderer BloomRenderer_;
	TextureRenderer TextureRenderer_;
};

#endif // VulkanWindow_h__
