#include "MeshNode.h"
#include "StaticMeshRenderer.h"

MeshNode::MeshNode(vk::Device device, StaticMeshRenderer* model, const aiMesh* mesh, const aiScene* scene, int memoryTypeIndex, aiMatrix4x4 matrix)
	: device_(device)
	, model_(model)
	, localMatrix_(matrix)
{
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	vertices.resize(mesh->mNumVertices);
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
		vertices[i] = vertex;
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}

	vertexBufferInfo_.range = sizeof(Vertex) * vertices.size();
	indexBufferInfo_.offset = vertexBufferInfo_.range;
	indexBufferInfo_.range = sizeof(unsigned int) * indices.size();

	vk::BufferCreateInfo bufferInfo;
	bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer;
	bufferInfo.size = vertexBufferInfo_.range + indexBufferInfo_.range;
	buffer_ = device.createBuffer(bufferInfo);
	indexBufferInfo_.buffer = vertexBufferInfo_.buffer = buffer_;

	vk::MemoryRequirements memReq = device.getBufferMemoryRequirements(buffer_);
	vk::MemoryAllocateInfo memAllocInfo(memReq.size, memoryTypeIndex);

	bufferMemory_ = device.allocateMemory(memAllocInfo);
	device.bindBufferMemory(buffer_, bufferMemory_, 0);

	uint8_t* bufferMemPtr = (uint8_t*)device.mapMemory(bufferMemory_, 0, memReq.size);
	memcpy(bufferMemPtr + vertexBufferInfo_.offset, vertices.data(), vertexBufferInfo_.range);
	memcpy(bufferMemPtr + indexBufferInfo_.offset, indices.data(), indexBufferInfo_.range);
	device.unmapMemory(bufferMemory_);
}

MeshNode::~MeshNode()
{
	device_.destroyBuffer(buffer_);
	device_.freeMemory(bufferMemory_);
}