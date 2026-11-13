#version 450
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;
void main() {
    vec3 top = vec3(0.02, 0.03, 0.10);
    vec3 bot = vec3(0.0, 0.0, 0.02);
    outColor = vec4(mix(bot, top, uv.y), 1.0);
}
