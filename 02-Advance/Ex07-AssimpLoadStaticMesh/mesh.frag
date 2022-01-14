#version 440

layout(location = 0) in vec2 TexCoords;
layout(location = 1) in vec3 vColor;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D diffuse;
//layout(binding = 2) uniform sampler2D specular;
//layout(binding = 3) uniform sampler2D ambient;
//layout(binding = 4) uniform sampler2D emissive;
//layout(binding = 5) uniform sampler2D height;
//layout(binding = 6) uniform sampler2D normals;
//layout(binding = 7) uniform sampler2D shininess;
//layout(binding = 8) uniform sampler2D opacity;
//layout(binding = 9) uniform sampler2D displacement;
//layout(binding = 10) uniform sampler2D lightmap;
//layout(binding = 11) uniform sampler2D reflection;
//layout(binding = 12) uniform sampler2D base_color;
//layout(binding = 13) uniform sampler2D normal_camera;
//layout(binding = 14) uniform sampler2D emission_color;
//layout(binding = 15) uniform sampler2D metalness;
//layout(binding = 16) uniform sampler2D diffuse_roughness;
//layout(binding = 17) uniform sampler2D ambient_occlusion;
//layout(binding = 19) uniform sampler2D sheen;
//layout(binding = 20) uniform sampler2D clearcoat;
//layout(binding = 21) uniform sampler2D transmission;

void main()
{
    fragColor = texture(diffuse,TexCoords);
}