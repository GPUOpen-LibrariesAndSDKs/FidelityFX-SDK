#version 450

#ifndef FAR_DEPTH
#define FAR_DEPTH 0
#endif

vec4 fullScreenVertsPos[3] = { vec4(-1, 1, FAR_DEPTH, 1), vec4(3, 1, FAR_DEPTH, 1), vec4(-1, -3, FAR_DEPTH, 1) };

void main()
{
    gl_Position = fullScreenVertsPos[gl_VertexIndex];
}
