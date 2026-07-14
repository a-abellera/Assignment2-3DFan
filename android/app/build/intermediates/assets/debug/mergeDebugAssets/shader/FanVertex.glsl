#version 300 es
layout(location = 0) in vec3 POSITION;
layout(location = 1) in float SHADE;

uniform mat4 MODELVIEWPROJECTIONMATRIX;

out float VarShade;

void main()
{
    gl_Position = MODELVIEWPROJECTIONMATRIX * vec4(POSITION, 1.0);
    VarShade = SHADE;
}
