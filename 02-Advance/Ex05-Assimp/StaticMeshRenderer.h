#ifndef StaticMeshRenderer_h__
#define StaticMeshRenderer_h__

#include <QVulkanWindow>
#include <vulkan\vulkan.hpp>
#include <assimp\Importer.hpp>
#include <map>
#include "MeshNode.h"
#include "QFPSCamera.h"

class StaticMeshRenderer : public QVulkanWindowRenderer {
	friend class MeshNode;
public:
	StaticMeshRenderer(QVulkanWindow* window);

	void loadFile(std::string file_path);
protected:
	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;
	void startNextFrame() override;
private:
	void processNode(const aiNode* node, const aiScene* scene, aiMatrix4x4 mat);
private:
	QVulkanWindow* window_ = nullptr;
	vk::Device device;
	Assimp::Importer importer_;
	std::vector<std::shared_ptr<MeshNode>> meshes_;

	vk::PipelineCache piplineCache_;
	vk::PipelineLayout piplineLayout_;
	vk::Pipeline pipline_;

	QFPSCamera camera_;
};

#endif // StaticMeshRenderer_h__
