#include "QVkWindow.h"
#include <memory>

void QVkScene::addRenderer(std::shared_ptr<QVkRenderer> renderer)
{
	rendererList_.push_back(renderer);
	renderer->setWindow(window_);
	if (window_->device()) {
		renderer->initResources();
		renderer->initSwapChainResources();
	}
}

void QVkScene::removeRenderer(std::shared_ptr<QVkRenderer> renderer)
{
	rendererList_.remove(renderer);
}

void QVkScene::initResources()
{
	for (auto& renderer : rendererList_) {
		renderer->initResources();
	}
}

void QVkScene::releaseResources()
{
	for (auto& renderer : rendererList_) {
		renderer->releaseResources();
	}
}

void QVkScene::initSwapChainResources()
{
	for (auto& renderer : rendererList_) {
		renderer->initSwapChainResources();
	}
}

void QVkScene::releaseSwapChainResources()
{
	for (auto& renderer : rendererList_) {
		renderer->releaseSwapChainResources();
	}
}

void QVkScene::startNextFrame()
{
	for (auto& renderer : rendererList_) {
		renderer->startNextFrame();
	}
	window_->frameReady();
	window_->requestUpdate();
}