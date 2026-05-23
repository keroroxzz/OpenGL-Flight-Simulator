#version 430 core

uniform sampler3D fluidTex;
uniform float speedScale;
uniform vec3 gridMin;
uniform vec3 gridMax;
uniform int mode; // 0 = Velocity, 1 = Vorticity

in vec3 gridPos;
out vec4 fragColor;

void main() {
    ivec3 size = textureSize(fluidTex, 0);
    vec3 range = gridMax - gridMin;
    vec3 texCoord = (gridPos - gridMin) / range;
    
    if (any(lessThan(texCoord, vec3(0))) || any(greaterThan(texCoord, vec3(1)))) {
        discard;
    }

    if (mode == 1) { // Vorticity
        vec3 eps = 1.0 / vec3(size);
        vec3 v_px = texture(fluidTex, texCoord + vec3(eps.x, 0, 0)).xyz;
        vec3 v_mx = texture(fluidTex, texCoord - vec3(eps.x, 0, 0)).xyz;
        vec3 v_py = texture(fluidTex, texCoord + vec3(0, eps.y, 0)).xyz;
        vec3 v_my = texture(fluidTex, texCoord - vec3(0, eps.y, 0)).xyz;
        vec3 v_pz = texture(fluidTex, texCoord + vec3(0, 0, eps.z)).xyz;
        vec3 v_mz = texture(fluidTex, texCoord - vec3(0, 0, eps.z)).xyz;
        
        vec3 curl;
        curl.x = (v_pz.y - v_mz.y) - (v_py.z - v_my.z);
        curl.y = (v_px.z - v_mx.z) - (v_pz.x - v_mz.x);
        curl.z = (v_py.x - v_my.x) - (v_px.y - v_mx.y);
        
        float mag = length(curl) * 50.0; // Scale curl for visibility
        fragColor = vec4(vec3(0, mag, mag*0.5), 0.8);
    } else { // Velocity
        vec3 v = texture(fluidTex, texCoord).rgb;
        float mag = length(v);
        float t = clamp(mag / (speedScale + 0.5), 0.0, 1.0);

        vec3 heatmap;
        if (t < 0.1) heatmap = mix(vec3(0.1, 0.1, 0.2), vec3(0.0, 0.0, 1.0), t/0.1);
        else if (t < 0.3) heatmap = mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), (t-0.1)/0.2);
        else if (t < 0.6) heatmap = mix(vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 0.0), (t-0.3)/0.3);
        else if (t < 0.8) heatmap = mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), (t-0.6)/0.2);
        else heatmap = mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), (t-0.8)/0.2);
        
        fragColor = vec4(heatmap, 0.8);
    }
}