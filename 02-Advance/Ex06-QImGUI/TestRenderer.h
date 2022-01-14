#ifndef TestRenderer_h__
#define TestRenderer_h__

#include "QImGUIRenderer.h"

class TestRenderer :public QImGUIRenderer {
public:
	TestRenderer(QVulkanWindow* window);
protected:
	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;
	void startNextFrame() override;
};

#endif // TestRenderer_h__
