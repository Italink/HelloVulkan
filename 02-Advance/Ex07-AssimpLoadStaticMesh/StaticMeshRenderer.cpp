#include "StaticMeshRenderer.h"

StaticMeshRenderer::StaticMeshRenderer(QVulkanWindow* window)
	: window_(window)
	, staticMesh_(window, PROJECT_SOURCE_DIR "/Genji/Genji.FBX")
{
	camera_.setup(window);
}

void StaticMeshRenderer::initResources()
{
	staticMesh_.initVulkanResource();
}

void StaticMeshRenderer::initSwapChainResources()
{
}

void StaticMeshRenderer::releaseSwapChainResources()
{
}

void StaticMeshRenderer::releaseResources()
{
	staticMesh_.releaseVulkanResource();
}

void StaticMeshRenderer::startNextFrame()
{
	vk::CommandBuffer cmdBuffer = window_->currentCommandBuffer();
	const QSize size = window_->swapChainImageSize();

	vk::ClearValue clearValues[3] = {
		vk::ClearColorValue(std::array<float,4>{0.0f,0.0f,0.0f,1.0f }),
		vk::ClearDepthStencilValue(1.0f,0),
		vk::ClearColorValue(std::array<float,4>{ 0.0f,0.5f,0.9f,1.0f }),
	};

	vk::RenderPassBeginInfo beginInfo;
	beginInfo.renderPass = window_->defaultRenderPass();
	beginInfo.framebuffer = window_->currentFramebuffer();
	beginInfo.renderArea.extent.width = size.width();
	beginInfo.renderArea.extent.height = size.height();
	beginInfo.clearValueCount = window_->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
	beginInfo.pClearValues = clearValues;

	cmdBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

	vk::Viewport viewport;
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = size.width();
	viewport.height = size.height();

	viewport.minDepth = 0;
	viewport.maxDepth = 1;
	cmdBuffer.setViewport(0, viewport);

	vk::Rect2D scissor;
	scissor.offset.x = scissor.offset.y = 0;
	scissor.extent.width = size.width();
	scissor.extent.height = size.height();
	cmdBuffer.setScissor(0, scissor);

	staticMesh_.makeRenderCommand(cmdBuffer, camera_.getMatrix());

	cmdBuffer.endRenderPass();

	window_->frameReady();
	window_->requestUpdate();
}