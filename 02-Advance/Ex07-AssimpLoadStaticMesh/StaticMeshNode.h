#ifndef StaticMeshNode_h__
#define StaticMeshNode_h__

#include <vulkan\vulkan.hpp>
#include "assimp\mesh.h"
#include "assimp\scene.h"

class StaticMesh;

class StaticMeshNode {
public:
	StaticMeshNode(StaticMesh* model, const aiMesh* mesh, const aiScene* scene, aiMatrix4x4 matrix);
	~StaticMeshNode();
	struct Texture {
		std::string path;
		aiTextureType type;
		vk::Image image;
		vk::ImageView imageView;
		vk::DeviceMemory imageMemory;
		vk::Sampler sampler;
	};
	struct Vertex {
		aiVector3D position;
		aiVector3D normal;
		aiVector3D tangent;
		aiVector3D bitangent;
		aiVector2D texCoords;
	};
	void initVulkanResource(vk::Device device, int memoryTypeIndex);

	StaticMesh* model_;
	vk::Device device_;
	std::vector<Vertex> vertices_;
	std::vector<unsigned int> indices_;
	aiMatrix4x4 localMatrix_;
	uint32_t materialIndex_ = 0;

	vk::Buffer buffer_;
	vk::DeviceMemory bufferMemory_;

	vk::DescriptorBufferInfo vertexBufferInfo_;
	vk::DescriptorBufferInfo indexBufferInfo_;

	vk::DescriptorSet descSet_;
};

#endif // StaticMeshNode_h__
