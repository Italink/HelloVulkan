#ifndef TrianglePipline_h__
#define TrianglePipline_h__

#include <QVulkanWindow>
#include <vulkan\vulkan.hpp>

class TrianglePipline {
public:
	TrianglePipline(QVulkanWindow* window);
	void init();
	void render();
	void destroy();
private:
	QVulkanWindow* window_ = nullptr;

	vk::Buffer vertexBuffer_;
	vk::DeviceMemory vertexDevMemory_;

	vk::PipelineCache piplineCache_;
	vk::PipelineLayout piplineLayout_;
	vk::Pipeline pipline_;
};

#endif // TrianglePipline_h__
