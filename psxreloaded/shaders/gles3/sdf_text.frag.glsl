#version 300 es
precision mediump float;

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D textAtlas;
uniform vec4 color;

void main()
{
    float distance = texture(textAtlas, TexCoord).r;
    float alpha = smoothstep(0.4, 0.6, distance);

    FragColor = vec4(color.rgb, alpha * color.a);
}