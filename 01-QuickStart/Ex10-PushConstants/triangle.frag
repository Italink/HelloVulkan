#version 440
layout(location = 0) in vec3 v_color;
layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConstant{
    float time;
}pushConstant;

void main()
{
    fragColor = vec4(v_color*sin(pushConstant.time),1.0);
}
