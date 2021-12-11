#include "VulkanWindow.h"
#include <QFile>
#include <QDesktopServices>
#include <QUrl>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

VulkanRenderer::VulkanRenderer(QVulkanWindow* window)
	:window_(window)
{
}

void VulkanRenderer::initResources()
{
	QImage image =  generateImage({800,600});
	image.save("compute.png");
	QDesktopServices::openUrl(QUrl("file:", QUrl::TolerantMode));
}

void VulkanRenderer::initSwapChainResources(){

}

void VulkanRenderer::releaseSwapChainResources()
{

}

void VulkanRenderer::releaseResources(){
	vk::Device device = window_->device();
}

void VulkanRenderer::startNextFrame(){


}

vk::ShaderModule VulkanRenderer::loadShader(const QString& name)
{
	QFile file(name);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning("Failed to read shader %s", qPrintable(name));
		return nullptr;
	}
	QByteArray blob = file.readAll();
	file.close();
	vk::ShaderModuleCreateInfo shaderInfo;
	shaderInfo.codeSize = blob.size();
	shaderInfo.pCode = reinterpret_cast<const uint32_t*>(blob.constData());
	vk::Device device = window_->device();
	return device.createShaderModule(shaderInfo);
}

QImage VulkanRenderer::generateImage(QSize size)
{

	vk::Device device = window_->device();

	vk::ImageCreateInfo imageInfo;
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.format = vk::Format::eR8G8B8A8Unorm;
	imageInfo.extent.width = size.width();
	imageInfo.extent.height = size.height();
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	imageInfo.tiling = vk::ImageTiling::eOptimal;
	imageInfo.usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc;
	imageInfo.initialLayout = vk::ImageLayout::ePreinitialized;
	vk::Image image = device.createImage(imageInfo);

	vk::MemoryRequirements memReq = device.getImageMemoryRequirements(image);
	vk::MemoryAllocateInfo memAllocInfo(memReq.size, window_->deviceLocalMemoryIndex());
	vk::DeviceMemory imageMemory = device.allocateMemory(memAllocInfo);
	device.bindImageMemory(image, imageMemory, 0);

	vk::ImageViewCreateInfo imageViewInfo;
	imageViewInfo.components = { vk::ComponentSwizzle::eR,vk::ComponentSwizzle::eG ,vk::ComponentSwizzle::eB ,vk::ComponentSwizzle::eA };
	imageViewInfo.format = imageInfo.format;
	imageViewInfo.image = image;
	imageViewInfo.viewType = vk::ImageViewType::e2D;
	imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	imageViewInfo.subresourceRange.layerCount = 1;
	imageViewInfo.subresourceRange.levelCount = 1;
	vk::ImageView imageView = device.createImageView(imageViewInfo);

	vk::DescriptorSetLayoutBinding descSetLayoutBingding;
	descSetLayoutBingding.binding = 0;
	descSetLayoutBingding.descriptorType = vk::DescriptorType::eStorageImage;
	descSetLayoutBingding.descriptorCount = 1;
	descSetLayoutBingding.stageFlags = vk::ShaderStageFlagBits::eCompute;
	descSetLayoutBingding.pImmutableSamplers = nullptr;

	vk::DescriptorSetLayoutCreateInfo descSetLayoutInfo;
	descSetLayoutInfo.bindingCount = 1;
	descSetLayoutInfo.pBindings = &descSetLayoutBingding;
	vk::DescriptorSetLayout descSetLayout = device.createDescriptorSetLayout(descSetLayoutInfo);

	vk::DescriptorPoolSize descPoolSize;
	descPoolSize.type = vk::DescriptorType::eStorageImage;
	descPoolSize.descriptorCount = 1;

	vk::DescriptorPoolCreateInfo descPoolInfo;
	descPoolInfo.maxSets = 1;
	descPoolInfo.poolSizeCount = 1;
	descPoolInfo.pPoolSizes = &descPoolSize;
	vk::DescriptorPool descPool = device.createDescriptorPool(descPoolInfo);

	vk::DescriptorSetAllocateInfo descSetAllocInfo;
	descSetAllocInfo.descriptorPool = descPool;
	descSetAllocInfo.descriptorSetCount = 1;
	descSetAllocInfo.pSetLayouts = &descSetLayout;
	vk::DescriptorSet descSet = device.allocateDescriptorSets(descSetAllocInfo).front();

	vk::DescriptorImageInfo descImageInfo(nullptr, imageView, vk::ImageLayout::eGeneral);
	vk::WriteDescriptorSet writeDescSet;
	writeDescSet.dstSet = descSet;
	writeDescSet.dstBinding = 0;
	writeDescSet.descriptorCount = 1;
	writeDescSet.descriptorType = descSetLayoutBingding.descriptorType;
	writeDescSet.pImageInfo = &descImageInfo;

	device.updateDescriptorSets(1, &writeDescSet, 0, nullptr);

	vk::PipelineLayoutCreateInfo piplineLayoutInfo;
	piplineLayoutInfo.setLayoutCount = 1;
	piplineLayoutInfo.pSetLayouts = &descSetLayout;
	vk::PipelineLayout piplineLayout = device.createPipelineLayout(piplineLayoutInfo);

	vk::PipelineCache piplineCache = device.createPipelineCache(vk::PipelineCacheCreateInfo());

	vk::ShaderModule computeShaderModule = loadShader("compute_image_comp.spv");

	vk::PipelineShaderStageCreateInfo computeStageInfo;
	computeStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
	computeStageInfo.module = computeShaderModule;
	computeStageInfo.pName = "main";

	vk::ComputePipelineCreateInfo computePiplineInfo;
	computePiplineInfo.stage = computeStageInfo;
	computePiplineInfo.layout = piplineLayout;
	vk::Pipeline computePipline = device.createComputePipeline(piplineCache, computePiplineInfo).value;

	vk::CommandBufferAllocateInfo cmdBufferAlllocInfo;
	cmdBufferAlllocInfo.level = vk::CommandBufferLevel::ePrimary;
	cmdBufferAlllocInfo.commandPool = window_->graphicsCommandPool();
	cmdBufferAlllocInfo.commandBufferCount = 1;
	vk::CommandBuffer cmdBuffer = device.allocateCommandBuffers(cmdBufferAlllocInfo).front();

	vk::CommandBufferBeginInfo cmdBeginInfo;
	cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	cmdBuffer.begin(cmdBeginInfo);

	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = vk::ImageLayout::eUndefined;
	barrier.newLayout = vk::ImageLayout::eGeneral;
	barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite;
	barrier.subresourceRange = imageViewInfo.subresourceRange;
	barrier.image = image;

	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr, 0, nullptr, 1, &barrier);
	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipline);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, piplineLayout, 0, 1, &descSet, 0, nullptr);
	cmdBuffer.dispatch(size.width() / 20, size.height() / 20, 1);

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

	barrier.image = image;
	barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
	barrier.oldLayout = vk::ImageLayout::eGeneral;
	barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
	barrier.subresourceRange = imageViewInfo.subresourceRange;
	cmdBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eComputeShader,
		vk::PipelineStageFlagBits::eTransfer,
		{},
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	barrier.image = stagingImage;
	barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
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
		1, &barrier
	);

	vk::ImageCopy imageCopy;
	imageCopy.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	imageCopy.srcSubresource.layerCount = 1;
	imageCopy.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	imageCopy.dstSubresource.layerCount = 1;
	imageCopy.extent.width = size.width();
	imageCopy.extent.height = size.height();
	imageCopy.extent.depth = 1;
	cmdBuffer.copyImage(image, vk::ImageLayout::eTransferSrcOptimal, stagingImage, vk::ImageLayout::eTransferDstOptimal, imageCopy);

	cmdBuffer.end();

	vk::Queue graphicsQueue = window_->graphicsQueue();
	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	graphicsQueue.submit(submitInfo);
	graphicsQueue.waitIdle();
	device.freeCommandBuffers(window_->graphicsCommandPool(), cmdBuffer);

	vk::ImageSubresource stagingSubRes;
	stagingSubRes.aspectMask = vk::ImageAspectFlagBits::eColor;
	stagingSubRes.arrayLayer = 0;
	stagingSubRes.mipLevel = 0;
	VkSubresourceLayout layout = device.getImageSubresourceLayout(stagingImage, stagingSubRes);
	uint8_t* memPtr = (uint8_t*)device.mapMemory(stagingImageMemory, layout.offset, layout.size, {});
	QImage readBackImage(size, QImage::Format::Format_RGBA8888);
	if (memPtr) {
		for (int y = 0; y < readBackImage.height(); ++y) {
			memcpy(readBackImage.scanLine(y), memPtr, readBackImage.width() * 4);
			memPtr += layout.rowPitch;
		}
		device.unmapMemory(stagingImageMemory);
	}

	device.destroyShaderModule(computeShaderModule);
	device.destroyImage(stagingImage);
	device.freeMemory(stagingImageMemory);
	device.destroyImageView(imageView);
	device.destroyImage(image);
	device.freeMemory(imageMemory);
	device.destroyDescriptorSetLayout(descSetLayout);
	device.destroyDescriptorPool(descPool);
	device.destroyPipelineLayout(piplineLayout);
	device.destroyPipeline(computePipline);
	device.destroyPipelineCache(piplineCache);


	return readBackImage;
}
