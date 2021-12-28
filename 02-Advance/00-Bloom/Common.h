#ifndef Common_h__
#define Common_h__

#include <QFile>
#include <vulkan/vulkan.hpp>

inline static vk::ShaderModule loadShader(const vk::Device& device, const QString& name) {
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
	return device.createShaderModule(shaderInfo);
}

#endif // Common_h__
