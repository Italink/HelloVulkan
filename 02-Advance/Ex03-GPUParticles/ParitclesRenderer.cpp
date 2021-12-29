#include "ParitclesRenderer.h"

ParticlesRenderer::ParticlesRenderer(QVulkanWindow* window)
	: particleSystem_(this)
	, window_(window)
{
}

void ParticlesRenderer::initResources()
{
	vk::Device device = window_->device();
	piplineCache_ = device.createPipelineCache(vk::PipelineCacheCreateInfo());
	particleSystem_.initResource();
}

void ParticlesRenderer::initSwapChainResources() {
}

void ParticlesRenderer::releaseSwapChainResources()
{
}

void ParticlesRenderer::releaseResources() {
	particleSystem_.releaseResource();
	vk::Device device = window_->device();
	device.destroyPipelineCache(piplineCache_);
}

void ParticlesRenderer::startNextFrame() {
	particleSystem_.create(100);
	particleSystem_.run();
	particleSystem_.render();
	window_->frameReady();
	window_->requestUpdate();
}