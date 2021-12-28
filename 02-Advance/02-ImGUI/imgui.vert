#version 440

layout(location = 0) in vec2 Position;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec4 Color;

layout(push_constant) uniform PushConstant{
   mat4 mvp;
}pushConstant;

layout(location = 0)out vec2 Frag_UV;
layout(location = 1)out vec4 Frag_Color;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
	Frag_UV = UV;
	Frag_Color = Color;
	gl_Position = pushConstant.mvp * vec4(Position.xy,0,1);
}
