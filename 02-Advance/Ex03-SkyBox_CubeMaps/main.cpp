#include <QGuiApplication>
#include <QLoggingCategory>
#include <QVulkanInstance>
#include <vulkan/vulkan.hpp>
#include "SkyBoxRenderer.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

Q_LOGGING_CATEGORY(lcVk, "qt.vulkan")

int main(int argc, char* argv[]) {
	QGuiApplication app(argc, argv);

	static vk::DynamicLoader  dynamicLoader;
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	QVulkanInstance instance;
	instance.setLayers({ "VK_LAYER_KHRONOS_validation" });
	if (!instance.create())
		qFatal("Failed to create Vulkan instance: %d", instance.errorCode());
	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance.vkInstance());

	QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));
	QVkWindow vkWindow;
	vkWindow.setVulkanInstance(&instance);
	vkWindow.resize(1024, 768);
	vkWindow.addRenderer(std::make_shared<SkyBoxRenderer>());
	vkWindow.show();
	return app.exec();
}