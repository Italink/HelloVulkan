#ifndef SkeletonMeshNode_h__
#define SkeletonMeshNode_h__

#include <vulkan\vulkan.hpp>
#include "assimp\mesh.h"
#include "assimp\scene.h"

class SkeletonMesh;

struct SkeletonBoneNode {
	std::string name;
	uint32_t boneIndex;
	aiMatrix4x4 localMatrix;
	aiMatrix4x4 offsetMatrix;
	std::vector<std::shared_ptr<SkeletonBoneNode>> children;
};

class SkeletonMeshNode {
public:
	SkeletonMeshNode(SkeletonMesh* model, const aiMesh* mesh, const aiScene* scene, aiMatrix4x4 matrix);
	~SkeletonMeshNode();

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
		std::array<int, 4> boneIndex = { -1,-1,-1,-1 };
		std::array<float, 4> boneWeight = { 0,0,0,0 };
	};

	void initVulkanResource(vk::Device device, int memoryTypeIndex);

	SkeletonMesh* model_;
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

#endif // SkeletonMeshNode_h__
