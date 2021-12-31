#version 440

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTangent;
layout (location = 3) in vec3 aBitangent;
layout (location = 4) in vec2 aTexCoords;

layout(push_constant) uniform PushConstant{
    mat4 mvp;
}pushConstant;

layout(location = 0) out vec2 vTexCoords;
layout(location = 1) out vec3 vColor;

out gl_PerVertex { vec4 gl_Position; };
 
void main()
{
    vTexCoords = aTexCoords;

    vColor = dot(aNormal,vec3(1,0,0)) * vec3(1) + vec3(0.5);

    gl_Position = pushConstant.mvp * vec4(aPos, 1.0);
}
