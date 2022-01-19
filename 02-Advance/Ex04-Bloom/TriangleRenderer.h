#ifndef TrianglePipline_h__
#define TrianglePipline_h__

#include "QVkWindow.h"

class TriangleRenderer :public QVkRenderer {
public:
	void initResources() override;
	void releaseResources() override;
	void startNextFrame() override;
private:
	vk::Buffer vertexBuffer_;
	vk::DeviceMemory vertexDevMemory_;
	vk::PipelineCache piplineCache_;
	vk::PipelineLayout piplineLayout_;
	vk::Pipeline pipline_;
};

#endif // TrianglePipline_h__
