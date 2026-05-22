#version 120
uniform sampler3D fluidTex;
uniform float speedScale;
uniform vec3 gridMin;
uniform vec3 gridMax;
varying vec3 gridPos;

void main() {
    vec3 range = gridMax - gridMin;
    vec3 texCoord = (gridPos - gridMin) / range;
    
    if (texCoord.x < 0.0 || texCoord.x > 1.0 || 
        texCoord.y < 0.0 || texCoord.y > 1.0 || 
        texCoord.z < 0.0 || texCoord.z > 1.0) {
        discard;
    }

    vec3 v = texture3D(fluidTex, texCoord).rgb;
    float mag = length(v);
    
    // High sensitivity heatmap
    float t = mag / (speedScale + 0.5);
    if (t > 1.0) t = 1.0;

    vec3 heatmap;
    if (t < 0.1) heatmap = mix(vec3(0.1, 0.1, 0.2), vec3(0.0, 0.0, 1.0), t/0.1);
    else if (t < 0.3) heatmap = mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), (t-0.1)/0.2);
    else if (t < 0.6) heatmap = mix(vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 0.0), (t-0.3)/0.3);
    else if (t < 0.8) heatmap = mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), (t-0.6)/0.2);
    else heatmap = mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), (t-0.8)/0.2);
    
    gl_FragColor = vec4(heatmap, 0.8);
}