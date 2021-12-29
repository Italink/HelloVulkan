#ifndef QVulkanMesh_h__
#define QVulkanMesh_h__

#include <vulkan\vulkan.hpp>

struct Vertex {
	float position[3];
	float normal[3];
	float texCoords[2];
	float tangent[3];
	float bitangent[3];
};

class VulkanMesh {
public:
	VulkanMesh(vk::Device device) :device_(device) {}
	vk::Device device_;
};

#endif // QVulkanMesh_h__
