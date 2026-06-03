#version 430 core

struct Particle {
    vec4 pos_life; // xyz = pos, w = normalized life
    vec4 vel_vort; // xyz = vel, w = vorticity magnitude
};

layout(std430, binding = 2) buffer particle_buf {
    Particle particles[];
};

uniform mat4 modelViewProj;
uniform float u_dt; 

out vec4 vColor;

uniform vec3 gridMin;
uniform vec3 gridMax;

void main() {
    uint particleIdx = gl_VertexID / 2;
    uint isTail = gl_VertexID % 2;
    
    Particle p = particles[particleIdx];
    float alpha = p.pos_life.w;
    float vort = p.vel_vort.w;
    
    // SAFETY: Detect invalid coordinates
    bool isInvalid = any(isnan(p.pos_life)) || any(isinf(p.pos_life)) || any(isnan(p.vel_vort)) || any(isinf(p.vel_vort));
    
    // Bounds check to discard particles that drifted too far
    vec3 center = (gridMin + gridMax) * 0.5;
    vec3 extent = (gridMax - gridMin) * 2.0; 
    bool isTooFar = any(greaterThan(abs(p.pos_life.xyz - center), extent));

    // Also check if the matrix itself is valid (can happen during fast camera moves)
    bool isMatrixInvalid = any(isnan(modelViewProj[0])) || any(isnan(modelViewProj[1])) || any(isnan(modelViewProj[2])) || any(isnan(modelViewProj[3]));

    if (alpha <= 0.01 || isInvalid || isTooFar || isMatrixInvalid) {
        // Discard BOTH vertices by moving to a point outside the clip volume [-1, 1]
        // Use w=1.0 to avoid division-by-zero artifacts
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        vColor = vec4(0.0);
        return;
    }

    vec3 pos = p.pos_life.xyz;
    if (isTail == 1) {
        pos -= p.vel_vort.xyz * u_dt * 50.0;
    }
    
    gl_Position = modelViewProj * vec4(pos, 1.0);
    
    // Final check to prevent NaNs from reaching the rasterizer
    if (any(isnan(gl_Position)) || any(isinf(gl_Position))) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        vColor = vec4(0.0);
        return;
    }

    float t_vort = clamp(vort / 8.0, 0.0, 1.0);
    float t_speed = clamp(length(p.vel_vort.xyz) / 40.0, 0.2, 1.0);
    
    vec3 baseColor = mix(vec3(0.0, 0.5, 1.0), vec3(0.1, 1.0, 0.8), t_speed);
    vec3 coreColor = vec3(1.0, 1.0, 1.0);
    vColor = vec4(mix(baseColor, coreColor, t_vort), alpha);
}
