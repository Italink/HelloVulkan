#include <QGuiApplication>
#include <QLoggingCategory>
#include <QVulkanInstance>
#include <vulkan/vulkan.hpp>
#include "ParitclesRenderer.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

Q_LOGGING_CATEGORY(lcVk, "qt.vulkan")

class VulkanWindow : public QVulkanWindow {
public:
	QVulkanWindowRenderer* createRenderer() override { return new ParticlesRenderer(this); }
};

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
	VulkanWindow vkWindow;
	vkWindow.setVulkanInstance(&instance);
	vkWindow.resize(1024, 768);
	vkWindow.show();
	return app.exec();
}