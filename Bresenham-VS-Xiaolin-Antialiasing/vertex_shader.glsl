// vertex_shader.glsl
#version 330 core
layout (location = 0) in vec2 aPos;
layout(location = 1) in float aAlpha;

out float alpha;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    gl_PointSize = 2.0; // pixel size
    alpha = aAlpha;
}