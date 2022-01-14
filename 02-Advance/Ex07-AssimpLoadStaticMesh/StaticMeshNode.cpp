#include "StaticMeshNode.h"
#include "StaticMesh.h"

StaticMeshNode::StaticMeshNode(StaticMesh* model, const aiMesh* mesh, const aiScene* scene, aiMatrix4x4 matrix)
	: model_(model)
	, localMatrix_(matrix)
	, materialIndex_(mesh->mMaterialIndex)
{
	vertices_.resize(mesh->mNumVertices);
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		Vertex vertex;
		vertex.position = mesh->mVertices[i];
		if (mesh->mNormals) {
			vertex.normal = mesh->mNormals[i];
		}
		if (mesh->mTextureCoords[0]) {
			vertex.texCoords.x = mesh->mTextureCoords[0][i].x;
			vertex.texCoords.y = mesh->mTextureCoords[0][i].y;
		}
		if (mesh->mTangents) {
			vertex.tangent = mesh->mTangents[i];
		}
		if (mesh->mBitangents) {
			vertex.bitangent = mesh->mBitangents[i];
		}
		vertices_[i] = vertex;
	}
	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++) {
			indices_.push_back(face.mIndices[j]);
		}
	}

	vertexBufferInfo_.range = sizeof(Vertex) * vertices_.size();
	indexBufferInfo_.offset = vertexBufferInfo_.range;
	indexBufferInfo_.range = sizeof(unsigned int) * indices_.size();
}

StaticMeshNode::~StaticMeshNode() {
	device_.destroyBuffer(buffer_);
	device_.freeMemory(bufferMemory_);
}

void StaticMeshNode::initVulkanResource(vk::Device device, int memoryTypeIndex)
{
	device_ = device;
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer;
	bufferInfo.size = vertexBufferInfo_.range + indexBufferInfo_.range;
	buffer_ = device_.createBuffer(bufferInfo);
	indexBufferInfo_.buffer = vertexBufferInfo_.buffer = buffer_;

	vk::MemoryRequirements memReq = device_.getBufferMemoryRequirements(buffer_);
	vk::MemoryAllocateInfo memAllocInfo(memReq.size, memoryTypeIndex);

	bufferMemory_ = device_.allocateMemory(memAllocInfo);
	device_.bindBufferMemory(buffer_, bufferMemory_, 0);

	uint8_t* bufferMemPtr = (uint8_t*)device_.mapMemory(bufferMemory_, 0, memReq.size);
	memcpy(bufferMemPtr + vertexBufferInfo_.offset, vertices_.data(), vertexBufferInfo_.range);
	memcpy(bufferMemPtr + indexBufferInfo_.offset, indices_.data(), indexBufferInfo_.range);
	device_.unmapMemory(bufferMemory_);

	if (materialIndex_ >= 0 && materialIndex_ < model_->textures_.size() && !model_->textures_[materialIndex_].empty()) {
		auto textures = model_->textures_[materialIndex_];

		vk::DescriptorSetAllocateInfo descSetAllocInfo(model_->descPool_, 1, &model_->descSetLayout_);
		descSet_ = device_.allocateDescriptorSets(descSetAllocInfo).front();

		std::vector<vk::WriteDescriptorSet>  descWrite(textures.size());
		for (int i = 0; i < textures.size(); i++) {
			vk::DescriptorImageInfo descImageInfo(textures[i]->sampler, textures[i]->imageView, vk::ImageLayout::eShaderReadOnlyOptimal);
			descWrite[i].dstSet = descSet_;
			descWrite[i].dstBinding = (uint32_t)textures[i]->type;
			descWrite[i].descriptorCount = 1;
			descWrite[i].descriptorType = vk::DescriptorType::eCombinedImageSampler;
			descWrite[i].pImageInfo = &descImageInfo;
		}
		device_.updateDescriptorSets(descWrite.size(), descWrite.data(), 0, nullptr);
	}
}