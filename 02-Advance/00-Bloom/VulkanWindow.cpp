#include "VulkanWindow.h"
#include <QFile>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

static float vertexData[] = { // Y up, front = CCW
	 0.0f,   -0.5f,   1.0f, 0.0f, 0.0f,
	-0.5f,    0.5f,   0.0f, 1.0f, 0.0f,
	 0.5f,    0.5f,   0.0f, 0.0f, 1.0f
};

TriangleRenderer::TriangleRenderer(QVulkanWindow* window)
	: window_(window)
	, trianglePipline_(window)
	, bloomPipline_(window)
	, fullScreenPipline_(window_)
{
	QList<int> sampleCounts = window->supportedSampleCounts();
	if (!sampleCounts.isEmpty()) {
		window->setSampleCount(sampleCounts.back());
	}
	window->resize(800, 600);
}

void TriangleRenderer::initResources() {
	trianglePipline_.init();
	bloomPipline_.initResource();
	fullScreenPipline_.init();
	fullScreenPipline_.updateImage(bloomPipline_.frameBuffer_[0].imageView, bloomPipline_.sampler_);
}

void TriangleRenderer::startNextFrame() {
	trianglePipline_.render();
	bloomPipline_.render();
	fullScreenPipline_.render();
	window_->frameReady();
	window_->requestUpdate();
}

void TriangleRenderer::releaseResources() {
	trianglePipline_.destroy();
	bloomPipline_.destroy();
	fullScreenPipline_.destroy();
}

void TriangleRenderer::initSwapChainResources()
{
	if (bloomPipline_.isInitialized()) {
		bloomPipline_.resizeFrameBuffer(window_->width(), window_->height());
		fullScreenPipline_.updateImage(bloomPipline_.frameBuffer_[0].imageView, bloomPipline_.sampler_);
	}
}

void TriangleRenderer::releaseSwapChainResources()
{
}