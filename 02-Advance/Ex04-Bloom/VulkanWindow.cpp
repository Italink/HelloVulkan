#include "VulkanWindow.h"

static float vertexData[] = { // Y up, front = CCW
	 0.0f,   -0.5f,   1.0f, 0.0f, 0.0f,
	-0.5f,    0.5f,   0.0f, 1.0f, 0.0f,
	 0.5f,    0.5f,   0.0f, 0.0f, 1.0f
};

TriangleRenderer::TriangleRenderer(QVulkanWindow* window)
	: window_(window)
	, trianglePipline_(window)
{
	QList<int> sampleCounts = window->supportedSampleCounts();
	if (!sampleCounts.isEmpty()) {
		window->setSampleCount(sampleCounts.back());
	}
	window->resize(800, 600);
}

void TriangleRenderer::initResources() {
	trianglePipline_.init();
	BloomRenderer_.init(window_->device());
	TextureRenderer_.init(window_->device(), (vk::SampleCountFlagBits)window_->sampleCountFlagBits(), window_->defaultRenderPass());
}

void TriangleRenderer::startNextFrame() {
	vk::CommandBuffer cmdBuffer = window_->currentCommandBuffer();
	trianglePipline_.render();
	BloomRenderer_.render(window_->swapChainImage(window_->currentSwapChainImageIndex()), window_->width(), window_->height(), cmdBuffer);

	vk::ClearValue clearValues[3] = {
		vk::ClearColorValue(std::array<float,4>{1.0f,0.0f,0.0f,1.0f }),
		vk::ClearDepthStencilValue(1.0f,0),
		vk::ClearColorValue(std::array<float,4>{ 0.0f,0.5f,0.9f,1.0f }),
	};

	vk::RenderPassBeginInfo beginInfo;
	beginInfo.renderPass = window_->defaultRenderPass();
	beginInfo.framebuffer = window_->currentFramebuffer();
	beginInfo.renderArea.extent.width = window_->width();
	beginInfo.renderArea.extent.height = window_->height();
	beginInfo.clearValueCount = window_->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
	beginInfo.pClearValues = clearValues;

	cmdBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

	vk::Viewport viewport;
	viewport.x = viewport.y = 0;
	viewport.width = window_->width();
	viewport.height = window_->height();
	viewport.minDepth = 0;
	viewport.maxDepth = 1;
	cmdBuffer.setViewport(0, viewport);

	vk::Rect2D scissor;
	scissor.offset.x = scissor.offset.y = 0;
	scissor.extent.width = viewport.width;
	scissor.extent.height = viewport.height;
	cmdBuffer.setScissor(0, scissor);

	TextureRenderer_.render(cmdBuffer);

	cmdBuffer.endRenderPass();

	window_->frameReady();
	window_->requestUpdate();
}

void TriangleRenderer::releaseResources() {
	trianglePipline_.destroy();
	BloomRenderer_.destroy();
	TextureRenderer_.destroy();
}

void TriangleRenderer::initSwapChainResources()
{
	BloomRenderer_.updateFrameBuffer(window_->width(), window_->height(), window_->deviceLocalMemoryIndex());
	TextureRenderer_.updateImage(BloomRenderer_.getOutputImageView());
}

void TriangleRenderer::releaseSwapChainResources()
{
}