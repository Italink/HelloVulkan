#version 440

layout(location = 0) in vec3 v_texcoord;

layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform samplerCube tex;

void main(){
    fragColor = texture(tex, v_texcoord);
}
