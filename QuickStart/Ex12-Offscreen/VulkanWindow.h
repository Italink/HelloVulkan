#ifndef VulkanWindow_h__
#define VulkanWindow_h__

#include <QVulkanWindowRenderer>
#include <vulkan\vulkan.hpp>
class VulkanRenderer: public QVulkanWindowRenderer {
public:
	VulkanRenderer(QVulkanWindow * window);
	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;
	void startNextFrame() override;
private:
	QImage createImage(QSize size,QColor color);
private:
	QVulkanWindow* window_ = nullptr;
};

class VulkanWindow: public QVulkanWindow{
public:
	QVulkanWindowRenderer* createRenderer() override{ return new VulkanRenderer(this); }
};

#endif // VulkanWindow_h__

