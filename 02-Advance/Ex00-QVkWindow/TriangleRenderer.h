#ifndef TriangleRenderer_h__
#define TriangleRenderer_h__

#include "QVkWindow.h"

class TriangleRenderer : public QVkRenderer {
public:
	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;
	void startNextFrame() override;
private:
	vk::Buffer vertexBuffer_;
	vk::DeviceMemory vertexDevMemory_;

	vk::PipelineCache piplineCache_;
	vk::PipelineLayout piplineLayout_;
	vk::Pipeline pipline_;
};

#endif // TriangleRenderer_h__