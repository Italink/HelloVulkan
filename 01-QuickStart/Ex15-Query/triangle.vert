#version 440

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 v_color;

layout(push_constant) uniform PushConstant{
    float time;
}pushConstant;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    v_color = color;
    gl_Position =  vec4(position * sin(pushConstant.time) ,0,1);
}
