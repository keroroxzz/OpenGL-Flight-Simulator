#version 430 core

struct Particle {
    vec4 pos_life; // xyz = pos, w = normalized life
    vec4 vel_vort; // xyz = vel, w = vorticity magnitude
};

layout(std430, binding = 0) buffer particle_buf {
    Particle particles[];
};

uniform mat4 modelViewProj;
uniform float tailLength; // Now interpreted as tail_dt (seconds)

out vec4 vColor;

void main() {
    uint particleIdx = gl_VertexID / 2;
    uint isTail = gl_VertexID % 2;
    
    Particle p = particles[particleIdx];
    float alpha = p.pos_life.w;
    float vort = p.vel_vort.w;
    
    if (alpha <= 0.01) {
        gl_Position = vec4(0,0,0,0);
        vColor = vec4(0);
        return;
    }

    if (vort < -50.0) {
        // NaN DIAGNOSTIC COLOR: Bright Red
        gl_Position = modelViewProj * vec4(p.pos_life.xyz, 1.0);
        vColor = vec4(1.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 pos = p.pos_life.xyz;
    if (isTail == 1) {
        // Tail is pos - v * tail_dt
        pos -= p.vel_vort.xyz * tailLength;
    }
    
    gl_Position = modelViewProj * vec4(pos, 1.0);
    
    float t_vort = clamp(vort / 8.0, 0.0, 1.0);
    float t_speed = clamp(length(p.vel_vort.xyz) / 40.0, 0.2, 1.0);
    
    vec3 baseColor = mix(vec3(0.0, 0.5, 1.0), vec3(0.1, 1.0, 0.8), t_speed);
    vec3 coreColor = vec3(1.0, 1.0, 1.0); // Pure white cores
    
    vColor = vec4(mix(baseColor, coreColor, t_vort), alpha * 1.0);
}
