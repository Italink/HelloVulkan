#include "QVulkanMesh.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>

void VulkanMesh::loadFile(std::string file_path) {
	const aiScene* scene = importer_.ReadFile("F:/QtVulkan/02-Advance/03-Assimp/Genji/Genji.FBX", aiProcess_Triangulate);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		printf("ERROR::ASSIMP:: %s", importer_.GetErrorString());
		return;
	}
}

void VulkanMesh::processNode(aiNode* node, aiScene* scene, aiMatrix4x4 mat)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		processNode(node->mChildren[i], scene, mat * node->mChildren[i]->mTransformation);
	}
}