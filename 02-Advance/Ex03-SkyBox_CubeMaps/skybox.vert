#version 440

layout(location = 0) in vec3 position;

layout(location = 0) out vec3 v_texcoord;

layout(push_constant) uniform PushConstant{
   mat4 mvp;
}pushConstant;

out gl_PerVertex { vec4 gl_Position; };

void main(){
    v_texcoord = position;
    gl_Position = pushConstant.mvp * vec4(position *1000 ,1.0);
}
