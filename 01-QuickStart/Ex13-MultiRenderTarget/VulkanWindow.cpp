#include "VulkanWindow.h"
#include <QDesktopServices>
#include <QUrl>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE


VulkanRenderer::VulkanRenderer(QVulkanWindow* window)
	:window_(window){
}

void VulkanRenderer::initResources(){	
	QVector<QImage> images = createImages({800,600}, {QColor(0, 100, 200), QColor(50, 200, 100),QColor(255,0,10),QColor(15,15,15)});
	for (int i = 0; i < images.size(); i++) {
		images[i].save("output"+QString::number(i)+".png");
	}
	QDesktopServices::openUrl(QUrl("file:"  , QUrl::TolerantMode));
}

void VulkanRenderer::initSwapChainResources()
{

}

void VulkanRenderer::releaseSwapChainResources()
{

}

void VulkanRenderer::releaseResources() {

}

void VulkanRenderer::startNextFrame() {
}


QVector<QImage> VulkanRenderer::createImages(QSize size, QVector<QColor> colors)
{
	if(colors.isEmpty())
		return {};
	vk::Device device = window_->device();
	QVector<vk::AttachmentDescription>  attachmentDesc(colors.size());
	attachmentDesc[0].format = vk::Format::eR8G8B8A8Unorm;
	attachmentDesc[0].samples = vk::SampleCountFlagBits::e1;
	attachmentDesc[0].loadOp = vk::AttachmentLoadOp::eClear;
	attachmentDesc[0].storeOp = vk::AttachmentStoreOp::eStore;
	attachmentDesc[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachmentDesc[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachmentDesc[0].initialLayout = vk::ImageLayout::eUndefined;
	attachmentDesc[0].finalLayout = vk::ImageLayout::eTransferSrcOptimal;
	for (int i = 1; i < attachmentDesc.size(); i++) {
		attachmentDesc[i] = attachmentDesc[0];
	}

	QVector<vk::AttachmentReference> attachmentRef(colors.size());
	for (int i = 0; i < attachmentRef.size(); i++) {
		attachmentRef[i].layout = vk::ImageLayout::eColorAttachmentOptimal;
		attachmentRef[i].attachment = i;
	}

	vk::SubpassDescription subpassDesc;
	subpassDesc.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpassDesc.colorAttachmentCount = attachmentRef.size();
	subpassDesc.pColorAttachments = attachmentRef.data();

	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.attachmentCount = attachmentDesc.size();
	renderPassInfo.pAttachments = attachmentDesc.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDesc;
	vk::RenderPass offscreenRenderPass = device.createRenderPass(renderPassInfo);

	QVector<FramebufferAttachment> colorAttachments(colors.size());


	vk::ImageCreateInfo attachmentImageInfo;
	attachmentImageInfo.imageType = vk::ImageType::e2D;
	attachmentImageInfo.format = vk::Format::eR8G8B8A8Unorm;
	attachmentImageInfo.extent.width = size.width();
	attachmentImageInfo.extent.height = size.height();
	attachmentImageInfo.extent.depth = 1;
	attachmentImageInfo.mipLevels = 1;
	attachmentImageInfo.arrayLayers = 1;
	attachmentImageInfo.samples = vk::SampleCountFlagBits::e1;
	attachmentImageInfo.tiling = vk::ImageTiling::eOptimal;
	attachmentImageInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;

	for (auto& colorAttachment : colorAttachments) {
		colorAttachment.image = device.createImage(attachmentImageInfo);
		vk::MemoryRequirements memReq = device.getImageMemoryRequirements(colorAttachment.image);
		vk::MemoryAllocateInfo memAllocInfo(memReq.size, window_->deviceLocalMemoryIndex());
		colorAttachment.imageMemroy = device.allocateMemory(memAllocInfo);
		device.bindImageMemory(colorAttachment.image, colorAttachment.imageMemroy, 0);

	}

	vk::ImageViewCreateInfo imageViewInfo;
	imageViewInfo.viewType = vk::ImageViewType::e2D;
	imageViewInfo.format = attachmentImageInfo.format;
	imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	imageViewInfo.subresourceRange.levelCount = imageViewInfo.subresourceRange.layerCount = 1;
	imageViewInfo.components.r = vk::ComponentSwizzle::eR;
	imageViewInfo.components.g = vk::ComponentSwizzle::eG;
	imageViewInfo.components.b = vk::ComponentSwizzle::eB;
	imageViewInfo.components.a = vk::ComponentSwizzle::eA;

	QVector<vk::ImageView> attachmentImageViews(colors.size());
	for (int i = 0; i < colorAttachments.size(); i++) {
		imageViewInfo.image = colorAttachments[i].image;
		colorAttachments[i].imageView = device.createImageView(imageViewInfo);
		attachmentImageViews[i] = colorAttachments[i].imageView;
	}

	vk::FramebufferCreateInfo framebufferInfo;
	framebufferInfo.renderPass = offscreenRenderPass;
	framebufferInfo.attachmentCount = attachmentImageViews.size();
	framebufferInfo.pAttachments = attachmentImageViews.data();
	framebufferInfo.width = size.width();
	framebufferInfo.height = size.height();
	framebufferInfo.layers = 1;
	vk::Framebuffer frameBuffer = device.createFramebuffer(framebufferInfo);

	vk::ImageCreateInfo stagingImageInfo;
	stagingImageInfo.imageType = vk::ImageType::e2D;
	stagingImageInfo.format = vk::Format::eR8G8B8A8Unorm;
	stagingImageInfo.extent.width = size.width();
	stagingImageInfo.extent.height = size.height();
	stagingImageInfo.extent.depth = 1;
	stagingImageInfo.mipLevels = 1;
	stagingImageInfo.arrayLayers = 1;
	stagingImageInfo.samples = vk::SampleCountFlagBits::e1;
	stagingImageInfo.tiling = vk::ImageTiling::eLinear;
	stagingImageInfo.usage = vk::ImageUsageFlagBits::eTransferDst;
	stagingImageInfo.initialLayout = vk::ImageLayout::eUndefined;
	vk::Image stagingImage = device.createImage(stagingImageInfo);

	vk::MemoryRequirements stagingMemReq = device.getImageMemoryRequirements(stagingImage);
	vk::MemoryAllocateInfo stagingBufferAllocInfo(stagingMemReq.size, window_->hostVisibleMemoryIndex());
	vk::DeviceMemory stagingImageMemory = device.allocateMemory(stagingBufferAllocInfo);
	device.bindImageMemory(stagingImage, stagingImageMemory, 0);

	vk::ImageSubresourceRange stagingSubResRange;
	stagingSubResRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	stagingSubResRange.baseMipLevel = 0;
	stagingSubResRange.levelCount = 1;
	stagingSubResRange.baseArrayLayer = 0;
	stagingSubResRange.layerCount = 1;

	vk::CommandBufferAllocateInfo cmdBufferAlllocInfo;
	cmdBufferAlllocInfo.level = vk::CommandBufferLevel::ePrimary;
	cmdBufferAlllocInfo.commandPool = window_->graphicsCommandPool();
	cmdBufferAlllocInfo.commandBufferCount = 1;
	vk::CommandBuffer cmdBuffer = device.allocateCommandBuffers(cmdBufferAlllocInfo).front();
	vk::CommandBufferBeginInfo cmdBufferBeginInfo;
	cmdBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue;
	cmdBuffer.begin(cmdBufferBeginInfo);

	QVector<vk::ClearValue> clearValues(colors.size());
	for (int i = 0; i < clearValues.size(); i++) {
		clearValues[i] = vk::ClearColorValue(std::array<float, 4>{colors[i].redF(), colors[i].greenF(), colors[i].blueF(), colors[i].alphaF() });
	}

	vk::RenderPassBeginInfo beginInfo;
	beginInfo.renderPass = offscreenRenderPass;
	beginInfo.framebuffer = frameBuffer;
	beginInfo.renderArea.extent.width = size.width();
	beginInfo.renderArea.extent.height = size.height();
	beginInfo.clearValueCount = clearValues.size();
	beginInfo.pClearValues = clearValues.data();

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
	cmdBuffer.endRenderPass();

	vk::ImageMemoryBarrier barrier;
	barrier.image = stagingImage;
	barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
	barrier.oldLayout = vk::ImageLayout::eUndefined;
	barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.subresourceRange = stagingSubResRange;
	cmdBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eTransfer,
		{},
		0, nullptr,
		0, nullptr,
		1, & barrier
	);
	cmdBuffer.end();

	vk::Queue graphicsQueue = window_->graphicsQueue();
	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	graphicsQueue.submit(submitInfo);
	graphicsQueue.waitIdle();

	vk::ImageSubresource stagingSubRes;
	stagingSubRes.aspectMask = vk::ImageAspectFlagBits::eColor;
	stagingSubRes.arrayLayer = 0;
	stagingSubRes.mipLevel = 0;

	vk::ImageCopy imageCopy;
	imageCopy.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	imageCopy.srcSubresource.layerCount = 1;
	imageCopy.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	imageCopy.dstSubresource.layerCount = 1;
	imageCopy.extent.width = size.width();
	imageCopy.extent.height = size.height();
	imageCopy.extent.depth = 1;

	VkSubresourceLayout layout = device.getImageSubresourceLayout(stagingImage, stagingSubRes);

	QVector<QImage> images;
	QImage readBackImage(size, QImage::Format::Format_RGBA8888);

	for (int i = 0; i < colors.size(); i++) {
		cmdBuffer = device.allocateCommandBuffers(cmdBufferAlllocInfo).front();
		vk::CommandBufferBeginInfo cmdBufferBeginInfo;
		cmdBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		cmdBuffer.begin(cmdBufferBeginInfo);
		cmdBuffer.copyImage(colorAttachments[i].image, vk::ImageLayout::eTransferSrcOptimal, stagingImage, vk::ImageLayout::eTransferDstOptimal, imageCopy);
		cmdBuffer.end();
		graphicsQueue.submit(submitInfo);
		graphicsQueue.waitIdle();

		uint8_t* memPtr = (uint8_t*)device.mapMemory(stagingImageMemory, layout.offset, layout.size, {});
		if (memPtr) {
			for (int y = 0; y < readBackImage.height(); ++y) {
				memcpy(readBackImage.scanLine(y), memPtr, readBackImage.width() * 4);
				memPtr += layout.rowPitch;
			}
			device.unmapMemory(stagingImageMemory);
		}
		else {
			qWarning("QVulkanWindow: Failed to map readback image memory after transfer");
		}
		images.push_back(readBackImage);
	}

	device.freeCommandBuffers(window_->graphicsCommandPool(), cmdBuffer);
	for (int i = 0; i < colorAttachments.size(); i++) {
		device.destroyImageView(colorAttachments[i].imageView);
		device.destroyImage(colorAttachments[i].image);
		device.freeMemory(colorAttachments[i].imageMemroy);
	}
	device.destroyImage(stagingImage);
	device.freeMemory(stagingImageMemory);
	device.destroyFramebuffer(frameBuffer);
	device.destroyRenderPass(offscreenRenderPass);

	return images;
}
