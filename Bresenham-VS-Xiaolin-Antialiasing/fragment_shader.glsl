// fragment_shader.glsl
#version 330 core
out vec4 FragColor;

in float alpha;

uniform float time;
uniform vec3 uColor;

void main()
{
    FragColor = vec4(uColor, alpha);
}