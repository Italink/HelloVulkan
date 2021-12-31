#include "SkeletonMeshRenderer.h"

SkeletonMeshRenderer::SkeletonMeshRenderer(QVulkanWindow* window)
	: window_(window)
	, staticMesh_(window, "F:/QtVulkan/02-Advance/Ex05-Assimp/Genji/Genji.FBX")
{
	camera_.setup(window);
}

void SkeletonMeshRenderer::initResources()
{
	staticMesh_.initVulkanResource();
}

void SkeletonMeshRenderer::initSwapChainResources()
{
}

void SkeletonMeshRenderer::releaseSwapChainResources()
{
}

void SkeletonMeshRenderer::releaseResources()
{
	staticMesh_.releaseVulkanResource();
}

void SkeletonMeshRenderer::startNextFrame()
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