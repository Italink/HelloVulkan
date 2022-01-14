#ifndef SkeletonMesh_h__
#define SkeletonMesh_h__

#include <vulkan\vulkan.hpp>
#include <assimp\Importer.hpp>
#include <map>
#include "SkeletonMeshNode.h"
#include "QFpsCamera.h"
#include "QVulkanWindow"
#include "SkeletonAnimation.h"

class SkeletonMesh {
	friend class SkeletonMeshNode;
public:
	SkeletonMesh(QVulkanWindow* window, std::string file_path);
	void initVulkanResource();
	void releaseVulkanResource();
	void makeRenderCommand(vk::CommandBuffer& cmdBuffer, QMatrix4x4 matrix);
protected:
	void initVulkanTexture();
	void initVulkanDescriptor();
	void initVulkanPipline();
private:
	std::shared_ptr<SkeletonBoneNode> processBoneNode(aiNode* node);
	void processNode(const aiNode* node, const aiScene* scene, aiMatrix4x4 mat);
	void processAnimations(const aiScene* scene);
	void processMaterialTextures(const aiScene* scene);
private:
	QVulkanWindow* window_;
	vk::Device device_;
	Assimp::Importer importer_;
	std::string meshPath_;
	const aiScene* scene = nullptr;
	std::vector<std::shared_ptr<SkeletonMeshNode>> meshes_;

	inline static std::vector<aiTextureType> textureTypes_ = { aiTextureType_DIFFUSE };
	std::map<std::string, std::shared_ptr<SkeletonMeshNode::Texture>> textureSet_;
	std::vector<std::vector<std::shared_ptr<SkeletonMeshNode::Texture>>> textures_;
	vk::Sampler commonSampler_;
	vk::DescriptorPool descPool_;
	vk::DescriptorSetLayout descSetLayout_;

	std::shared_ptr<SkeletonBoneNode> boneRoot_;
	std::map<std::string, std::shared_ptr<SkeletonBoneNode>> boneSet_;

	std::vector<std::shared_ptr<SkeletonAnimation>> skeletonAnimations_;

	vk::PipelineCache piplineCache_;
	vk::PipelineLayout piplineLayout_;
	vk::Pipeline pipline_;
};

#endif // SkeletonMesh_h__
