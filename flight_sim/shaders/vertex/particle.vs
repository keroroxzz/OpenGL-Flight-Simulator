#version 430 core

struct Particle {
    vec4 pos_life; // xyz = WORLD pos, w = normalized life
    vec4 vel_vort; // xyz = vel, w = vorticity magnitude
};

layout(std430, binding = 0) buffer particle_buf {
    Particle particles[];
};

uniform mat4 modelViewProj;
uniform vec3 gridMin;
uniform vec3 gridMax;
uniform float u_dt; 

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

    vec3 worldPos = p.pos_life.xyz;

    if (vort < -50.0) {
        gl_Position = modelViewProj * vec4(worldPos, 1.0);
        vColor = vec4(1.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 pos = worldPos;
    if (isTail == 1) {
        pos -= p.vel_vort.xyz * u_dt * 50.0;
    }
    
    gl_Position = modelViewProj * vec4(pos, 1.0);
    
    float t_vort = clamp(vort / 8.0, 0.0, 1.0);
    float t_speed = clamp(length(p.vel_vort.xyz) / 40.0, 0.2, 1.0);
    
    vec3 baseColor = mix(vec3(0.0, 0.5, 1.0), vec3(0.1, 1.0, 0.8), t_speed);
    vec3 coreColor = vec3(1.0, 1.0, 1.0);
    
    vColor = vec4(mix(baseColor, coreColor, t_vort), alpha);
}
