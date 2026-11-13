#version 450
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;
float hash(vec2 p){ return fract(sin(dot(p,vec2(12.9898,78.233)))*43758.5453); }
float noise(vec2 p){ vec2 i=floor(p); vec2 f=fract(p); f=f*f*(3.0-2.0*f); float a=hash(i); float b=hash(i+vec2(1,0)); float c=hash(i+vec2(0,1)); float d=hash(i+vec2(1,1)); return mix(mix(a,b,f.x),mix(c,d,f.x),f.y); }
float fbm(vec2 p){ float v=0.0; float a=0.5; for(int i=0;i<5;i++){ v+=a*noise(p); p*=2.0; a*=0.5; } return v; }
void main() {
    vec2 p = uv * 4.0;
    float n = fbm(p);
    vec3 col = mix(vec3(0.1,0.0,0.2), vec3(0.6,0.3,0.8), n);
    outColor = vec4(col * 0.6, 0.7);
}
