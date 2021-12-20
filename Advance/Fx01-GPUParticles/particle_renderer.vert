#version 440

layout(location = 0 ) in vec3 position;

layout(push_constant) uniform PushConstant{
    mat4 view;
}pushConstant;


out gl_PerVertex { 
    vec4 gl_Position;
    float gl_PointSize;
};

void main(){
    gl_Position =  pushConstant.view * vec4(position,1);
    gl_PointSize = 2;
}
