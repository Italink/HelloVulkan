#version 440

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 v_color;

layout(push_constant) uniform Constant{
    float factor;
}constant;

out gl_PerVertex { vec4 gl_Position; };

void main(){
    if(constant.factor>1.0){
        v_color = vec3(0);
    }else{
        v_color = color;
    }
    gl_Position =  vec4(position * constant.factor, 0, 1);
}
