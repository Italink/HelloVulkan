#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <filesystem>

#include <SPIRV/GlslangToSpv.h>
#include <glslang/Public/ShaderLang.h>

#include "shader_vert.inl"
#include "shader_frag.inl"
#include "ResourceLimits.h"

using namespace std;

std::vector<uint32_t> GLSLtoSPV(std::string filePath, EShLanguage shaderType) {
	std::vector<uint32_t> spvShader;
	if (!filesystem::exists(filePath))
		return spvShader;
	ifstream input(filePath);
	std::string glslShader;
	getline(input, glslShader, '\0');	//read all
	glslang::TShader shader(shaderType);

	const char* shaderStrings[1] = { glslShader.data() };
	shader.setStrings(shaderStrings, 1);

	EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
	shader.parse(&glslang::DefaultTBuiltInResource, 100, false, messages);
	glslang::SpvOptions option;
	spv::SpvBuildLogger logger;
	glslang::GlslangToSpv(*shader.getIntermediate(), spvShader, &logger, &option);
	cout << logger.getAllMessages();
	return spvShader;
}

int main(int argc, char* argv[]) {
	glslang::InitializeProcess();
	std::vector<uint32_t> vertexShader = std::move(GLSLtoSPV(PROJECT_SOURCE_DIR"/shader.vert", EShLanguage::EShLangVertex));
	std::vector<uint32_t> fragmentShader = std::move(GLSLtoSPV(PROJECT_SOURCE_DIR"/shader.frag", EShLanguage::EShLangFragment));

	glslang::FinalizeProcess();
	assert(std::equal(vertexShader.begin(), vertexShader.end(), shader_vert));
	assert(std::equal(fragmentShader.begin(), fragmentShader.end(), shader_frag));

	return 0;
}