#version 450

layout (binding = 0) uniform sampler2D samplerColor;

layout(push_constant) uniform BlurParams{
	float scale;
	float strength ;
	int size;
	float weight[10];
}blurParams;

layout (constant_id = 0) const int blurdirection = 0;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec2 tex_offset = 1.0 / textureSize(samplerColor, 0) * blurParams.scale; // gets size of single texel
	vec4 raw = texture(samplerColor, inUV);
	vec4 result = raw * blurParams.weight[0]; // current fragment's contribution
	for(int i = 1; i < blurParams.size; ++i)
	{
		if (blurdirection == 1)
		{
			// H
			result += texture(samplerColor, inUV + vec2(tex_offset.x * i, 0.0)) * blurParams.weight[i];
			result += texture(samplerColor, inUV - vec2(tex_offset.x * i, 0.0)) * blurParams.weight[i];
		}
		else
		{
			// V
			result += texture(samplerColor, inUV + vec2(0.0, tex_offset.y * i)) * blurParams.weight[i];
			result += texture(samplerColor, inUV - vec2(0.0, tex_offset.y * i)) * blurParams.weight[i];
		}
	}

	vec3 hdrColor = result.rgb;
    vec3 mapped =vec3(1.0) - exp(-(hdrColor * blurParams.strength));
	mapped = pow(mapped, vec3(1.0 / blurParams.scale));

	float alpha = exp(-result.a * blurParams.strength);

    outFragColor = vec4(mix(mapped,raw.rgb * blurParams.strength , raw.a), alpha);
}