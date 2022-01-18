#ifndef VulkanWindow_h__
#define VulkanWindow_h__

#include <QVulkanWindowRenderer>
#include <vulkan\vulkan.hpp>

class VulkanRenderer : public QVulkanWindowRenderer {		//Vulkan 渲染器
public:
	VulkanRenderer(QVulkanWindow* window);

	void initResources() override;						//创建vulkan资源
	void releaseResources() override;					//释放vulkan资源

	void initSwapChainResources() override;				//创建交换链资源
	void releaseSwapChainResources() override;			//释放交换链资源

	void startNextFrame() override;						//开始下一帧的绘制

private:
	QVulkanWindow* window_;
};

#endif // VulkanWindow_h__
