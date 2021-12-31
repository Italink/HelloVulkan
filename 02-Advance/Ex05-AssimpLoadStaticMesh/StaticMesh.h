#ifndef StaticMesh_h__
#define StaticMesh_h__

#include <vulkan\vulkan.hpp>
#include <assimp\Importer.hpp>
#include <map>
#include "StaticMeshNode.h"
#include "QFpsCamera.h"
#include "QVulkanWindow"

class StaticMesh {
	friend class StaticMeshNode;
public:
	StaticMesh(QVulkanWindow* window, std::string file_path);
	void initVulkanResource();
	void releaseVulkanResource();
	void makeRenderCommand(vk::CommandBuffer& cmdBuffer, QMatrix4x4 matrix);
protected:
	void initVulkanTexture();
	void initVulkanDescriptor();
	void initVulkanPipline();
private:
	void processNode(const aiNode* node, const aiScene* scene, aiMatrix4x4 mat);
	void processMaterialTextures(const aiScene* scene);
private:
	QVulkanWindow* window_;
	vk::Device device_;
	Assimp::Importer importer_;
	std::string meshPath_;
	const aiScene* scene = nullptr;
	std::vector<std::shared_ptr<StaticMeshNode>> meshes_;

	inline static std::vector<aiTextureType> textureTypes_ = { aiTextureType_DIFFUSE };
	std::map<std::string, std::shared_ptr<StaticMeshNode::Texture>> textureSet_;
	std::vector<std::vector<std::shared_ptr<StaticMeshNode::Texture>>> textures_;
	vk::Sampler commonSampler_;
	vk::DescriptorPool descPool_;
	vk::DescriptorSetLayout descSetLayout_;

	vk::PipelineCache piplineCache_;
	vk::PipelineLayout piplineLayout_;
	vk::Pipeline pipline_;
};

#endif // StaticMesh_h__
