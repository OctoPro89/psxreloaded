#version 300 es

precision highp float;

uniform uvec2 uViewport;
uniform ivec4 uRect;
uniform vec4 uUV;

out vec2 vUV;

void main()
{
    int x = gl_VertexID & 1;
    int y = gl_VertexID >> 1;

    ivec2 pos;

    pos.x = uRect.x + x * uRect.z;
    pos.y = uRect.y + y * uRect.w;

    vec2 ndc;

    ndc.x = (float(pos.x) / float(uViewport.x)) * 2.0 - 1.0;

    pos.y = int(uViewport.y) - pos.y;

    ndc.y = (float(pos.y) / float(uViewport.y)) * 2.0 - 1.0;

    gl_Position = vec4(ndc, 0.0, 1.0);

    vUV.x = (x == 0) ? uUV.x : uUV.z;
    vUV.y = (y == 0) ? uUV.y : uUV.w;
}