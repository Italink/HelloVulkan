#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <filesystem>

#include "shader_vert.inl"
#include "shader_frag.inl"

#include <SPIRV/GlslangToSpv.h>
#include <glslang/Public/ShaderLang.h>
#include "ResourceLimits.h"

using namespace std;

const char* vertexCode = R"(
#version 440
layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;
layout(location = 0) out vec3 v_color;
out gl_PerVertex { vec4 gl_Position; };
void main(){
    v_color = color;
    gl_Position =  vec4(position,0,1);
}
)";

const char* fragmentCode = R"(
#version 440
layout(location = 0) in vec3 v_color;
layout(location = 0) out vec4 fragColor;
void main(){
    fragColor = vec4(v_color,1.0);
}
)";

std::vector<uint32_t> GLSLtoSPV(const char* code, EShLanguage shaderType) {
	std::vector<uint32_t> spvShader;
	glslang::TShader shader(shaderType);
	shader.setStrings(&code, 1);
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
	std::vector<uint32_t> vertexShader = std::move(GLSLtoSPV(vertexCode, EShLanguage::EShLangVertex));
	std::vector<uint32_t> fragmentShader = std::move(GLSLtoSPV(fragmentCode, EShLanguage::EShLangFragment));

	glslang::FinalizeProcess();
	assert(std::equal(vertexShader.begin(), vertexShader.end(), shader_vert));
	assert(std::equal(fragmentShader.begin(), fragmentShader.end(), shader_frag));
	return 0;
}