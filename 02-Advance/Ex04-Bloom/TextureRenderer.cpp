#include "TextureRenderer.h"
#include "full_screen_vert.inl"
#include "display_frag.inl"

TextureRenderer::TextureRenderer()
{}

void TextureRenderer::init(vk::Device device, vk::SampleCountFlagBits sampleCount, vk::RenderPass renderPass)
{
	this->device = device;

	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eNearest;
	samplerInfo.minFilter = vk::Filter::eNearest;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.maxAnisotropy = 1.0f;
	sampler_ = device.createSampler(samplerInfo);

	vk::DescriptorPoolSize descPoolSize(vk::DescriptorType::eCombinedImageSampler, 1);

	vk::DescriptorPoolCreateInfo descPoolInfo;
	descPoolInfo.maxSets = 1;
	descPoolInfo.poolSizeCount = 1;
	descPoolInfo.pPoolSizes = &descPoolSize;
	descPool_ = device.createDescriptorPool(descPoolInfo);

	vk::DescriptorSetLayoutBinding layoutBinding = { 0, vk::DescriptorType::eCombinedImageSampler,1,vk::ShaderStageFlagBits::eFragment };

	vk::DescriptorSetLayoutCreateInfo descLayoutInfo;
	descLayoutInfo.pNext = nullptr;
	descLayoutInfo.flags = {};
	descLayoutInfo.bindingCount = 1;
	descLayoutInfo.pBindings = &layoutBinding;

	descSetLayout_ = device.createDescriptorSetLayout(descLayoutInfo);

	vk::DescriptorSetAllocateInfo descSetAllocInfo(descPool_, 1, &descSetLayout_);
	descSet_ = device.allocateDescriptorSets(descSetAllocInfo).front();

	vk::GraphicsPipelineCreateInfo piplineInfo;
	piplineInfo.stageCount = 2;

	vk::ShaderModuleCreateInfo shaderInfo;
	shaderInfo.codeSize = sizeof(full_screen_vert);
	shaderInfo.pCode = full_screen_vert;
	vk::ShaderModule vertShader = device.createShaderModule(shaderInfo);

	shaderInfo.codeSize = sizeof(display_frag);
	shaderInfo.pCode = display_frag;
	vk::ShaderModule fragShader = device.createShaderModule(shaderInfo);

	vk::PipelineShaderStageCreateInfo piplineShaderStage[2];
	piplineShaderStage[0].stage = vk::ShaderStageFlagBits::eVertex;
	piplineShaderStage[0].module = vertShader;
	piplineShaderStage[0].pName = "main";
	piplineShaderStage[1].stage = vk::ShaderStageFlagBits::eFragment;
	piplineShaderStage[1].module = fragShader;
	piplineShaderStage[1].pName = "main";
	piplineInfo.pStages = piplineShaderStage;

	vk::PipelineVertexInputStateCreateInfo vertexInputState({}, 0, nullptr, 0, nullptr);
	piplineInfo.pVertexInputState = &vertexInputState;

	vk::PipelineInputAssemblyStateCreateInfo vertexAssemblyState({}, vk::PrimitiveTopology::eTriangleList);
	piplineInfo.pInputAssemblyState = &vertexAssemblyState;

	vk::PipelineViewportStateCreateInfo viewportState;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	piplineInfo.pViewportState = &viewportState;

	vk::PipelineRasterizationStateCreateInfo rasterizationState;
	rasterizationState.polygonMode = vk::PolygonMode::eFill;
	rasterizationState.cullMode = vk::CullModeFlagBits::eNone;
	rasterizationState.frontFace = vk::FrontFace::eCounterClockwise;
	rasterizationState.lineWidth = 1.0f;
	piplineInfo.pRasterizationState = &rasterizationState;

	vk::PipelineMultisampleStateCreateInfo MSState;
	MSState.rasterizationSamples = sampleCount;
	piplineInfo.pMultisampleState = &MSState;

	vk::PipelineDepthStencilStateCreateInfo DSState;
	DSState.depthTestEnable = false;
	DSState.depthWriteEnable = false;
	DSState.depthCompareOp = vk::CompareOp::eLessOrEqual;
	piplineInfo.pDepthStencilState = &DSState;

	vk::PipelineColorBlendStateCreateInfo colorBlendState;
	colorBlendState.attachmentCount = 1;
	vk::PipelineColorBlendAttachmentState colorBlendAttachmentState;
	colorBlendAttachmentState.blendEnable = true;
	colorBlendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	colorBlendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	colorBlendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	colorBlendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	colorBlendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	colorBlendState.pAttachments = &colorBlendAttachmentState;
	piplineInfo.pColorBlendState = &colorBlendState;

	vk::PipelineDynamicStateCreateInfo dynamicState;
	vk::DynamicState dynamicEnables[] = { vk::DynamicState::eViewport ,vk::DynamicState::eScissor };
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicEnables;
	piplineInfo.pDynamicState = &dynamicState;

	vk::PipelineLayoutCreateInfo piplineLayoutInfo;
	piplineLayoutInfo.setLayoutCount = 1;
	piplineLayoutInfo.pSetLayouts = &descSetLayout_;
	piplineLayout_ = device.createPipelineLayout(piplineLayoutInfo);
	piplineInfo.layout = piplineLayout_;

	piplineInfo.renderPass = renderPass;

	piplineCache_ = device.createPipelineCache(vk::PipelineCacheCreateInfo());

	pipline_ = device.createGraphicsPipeline(piplineCache_, piplineInfo).value;

	device.destroyShaderModule(vertShader);
	device.destroyShaderModule(fragShader);
}

void TextureRenderer::render(vk::CommandBuffer cmdBuffer)
{
	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipline_);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, piplineLayout_, 0, 1, &descSet_, 0, nullptr);
	cmdBuffer.draw(4, 1, 0, 0);
}

void TextureRenderer::destroy()
{
	device.destroySampler(sampler_);
	device.destroyDescriptorPool(descPool_);
	device.destroyDescriptorSetLayout(descSetLayout_);
	device.destroyPipeline(pipline_);
	device.destroyPipelineCache(piplineCache_);
	device.destroyPipelineLayout(piplineLayout_);
}

void TextureRenderer::updateImage(vk::ImageView image)
{
	vk::DescriptorImageInfo descImageInfo(sampler_, image, vk::ImageLayout::eShaderReadOnlyOptimal);
	vk::WriteDescriptorSet descWrite;
	descWrite.dstSet = descSet_;
	descWrite.dstBinding = 0;
	descWrite.descriptorCount = 1;
	descWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	descWrite.pImageInfo = &descImageInfo;
	device.updateDescriptorSets(1, &descWrite, 0, nullptr);
}

void TextureRenderer::updateRect(int x, int y, int width, int height)
{
}