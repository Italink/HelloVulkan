#ifndef VulkanWindow_h__
#define VulkanWindow_h__

#include <QVulkanWindowRenderer>
#include <vulkan\vulkan.hpp>
class TriangleRenderer: public QVulkanWindowRenderer {
public:
	TriangleRenderer(QVulkanWindow * window);
	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;
	void startNextFrame() override;
private:
	vk::ShaderModule loadShader(const QString& name);
private:
	QVulkanWindow* window_ = nullptr;

	vk::Queue computeQueue_;
};

class VulkanWindow: public QVulkanWindow{
public:
	QVulkanWindowRenderer* createRenderer() override{ return new TriangleRenderer(this); }
};

#endif // VulkanWindow_h__

