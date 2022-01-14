#ifndef OffscreenRenderer_h__
#define OffscreenRenderer_h__

#include <QVulkanWindowRenderer>
#include <vulkan\vulkan.hpp>

class OffscreenRenderer : public QVulkanWindowRenderer {
public:
	OffscreenRenderer(QVulkanWindow* window);
	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;
	void startNextFrame() override;
private:
	QImage createImage(QSize size, QColor color);
private:
	QVulkanWindow* window_ = nullptr;
};

#endif // OffscreenRenderer_h__
