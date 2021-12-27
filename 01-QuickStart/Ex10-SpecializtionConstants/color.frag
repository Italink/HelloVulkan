#version 440
layout(location = 0) in vec3 v_color;
layout(location = 0) out vec4 fragColor;

layout(constant_id = 0) const int SWITCHER = 0;

void main(){
    switch (SWITCHER){
        case 0:
            fragColor = vec4(v_color,1.0);
            break;
        case 1:
            fragColor = vec4(0,1,0,1);
            break;
    }
}
