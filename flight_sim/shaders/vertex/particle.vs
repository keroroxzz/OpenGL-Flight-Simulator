#version 430 core

struct Particle {
    vec4 pos_life; // xyz = pos, w = normalized life
    vec4 vel_maxLife; // xyz = vel, w = maxLife
};

layout(std430, binding = 0) buffer particle_buf {
    Particle particles[];
};

uniform mat4 modelViewProj;
uniform float tailLength;

out vec4 vColor;

void main() {
    uint particleIdx = gl_VertexID / 2;
    uint isTail = gl_VertexID % 2;
    
    Particle p = particles[particleIdx];
    float alpha = p.pos_life.w;
    
    vec3 pos = p.pos_life.xyz;
    if (isTail == 1) {
        pos -= p.vel_maxLife.xyz * tailLength;
    }
    
    gl_Position = modelViewProj * vec4(pos, 1.0);
    
    // Color based on velocity magnitude
    float speed = length(p.vel_maxLife.xyz);
    float t = clamp(speed / 40.0, 0.0, 1.0);
    
    if (t < 0.25) vColor = vec4(0, 4.0*t, 1, alpha * 0.6);
    else if (t < 0.5) vColor = vec4(0, 1, 1.0 - 4.0*(t-0.25), alpha * 0.6);
    else if (t < 0.75) vColor = vec4(4.0*(t-0.5), 1, 0, alpha * 0.6);
    else vColor = vec4(1, 1.0 - 4.0*(t-0.75), 0, alpha * 0.6);
}
