#ifndef MeshNode_h__
#define MeshNode_h__

#include <vulkan\vulkan.hpp>
#include "assimp\mesh.h"
#include "assimp\scene.h"

class StaticMeshRenderer;

class MeshNode {
public:
	MeshNode(vk::Device device, StaticMeshRenderer* model, const aiMesh* mesh, const aiScene* scene, int memoryTypeIndex, aiMatrix4x4 matrix);

	struct Texture {
		std::string path;
		aiTextureType type;
		vk::Image image;
		vk::ImageView imageView;
		vk::DeviceMemory imageMemory;
	};
	struct Vertex {
		aiVector3D position;
		aiVector3D normal;
		aiVector3D tangent;
		aiVector3D bitangent;
		aiVector2D texCoords;
	};

	StaticMeshRenderer* model_;
	vk::Device device_;
	aiMatrix4x4 localMatrix_;

	vk::Buffer buffer_;
	vk::DeviceMemory bufferMemory_;

	vk::DescriptorBufferInfo vertexBufferInfo_;
	vk::DescriptorBufferInfo indexBufferInfo_;

	std::vector<std::shared_ptr<Texture>> textures_;
};

#endif // MeshNode_h__
