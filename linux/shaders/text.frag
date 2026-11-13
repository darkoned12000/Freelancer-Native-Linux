#version 450
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D tex;
layout(push_constant) uniform PC { vec4 color; } pc;
void main() {
    float a = texture(tex, uv).r;
    outColor = vec4(pc.color.rgb, pc.color.a * a);
}
