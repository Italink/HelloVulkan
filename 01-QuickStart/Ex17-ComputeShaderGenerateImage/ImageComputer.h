#ifndef ImageComputer_h__
#define ImageComputer_h__

#include <QVulkanWindowRenderer>
#include <vulkan\vulkan.hpp>

class ImageComputer : public QVulkanWindowRenderer {
public:
	ImageComputer(QVulkanWindow* window);
	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;
	void startNextFrame() override;
private:
	QImage generateImage(QSize size);
private:
	QVulkanWindow* window_ = nullptr;
};

#endif // ImageComputer_h__
