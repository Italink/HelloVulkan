#ifndef MRTRenderer_h__
#define MRTRenderer_h__

#include <QVulkanWindowRenderer>
#include <vulkan\vulkan.hpp>

class MRTRenderer : public QVulkanWindowRenderer {
public:
	MRTRenderer(QVulkanWindow* window);
	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;
	void startNextFrame() override;
private:
	QVector<QImage> createImages(QSize size, QVector<QColor> colors);
private:
	QVulkanWindow* window_ = nullptr;
	struct FramebufferAttachment {
		vk::Image image;
		vk::DeviceMemory imageMemroy;
		vk::ImageView imageView;
	};
};

#endif // MRTRenderer_h__
