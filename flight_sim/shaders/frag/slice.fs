#version 120
uniform sampler3D fluidTex;
uniform float speedScale;
varying vec3 gridPos;

void main() {
    // New Mapping: X:[-22, 8], Y:[-10, 10], Z:[-5, 5]
    vec3 texCoord;
    texCoord.x = (gridPos.x + 22.0) / 30.0;
    texCoord.y = (gridPos.y + 10.0) / 20.0;
    texCoord.z = (gridPos.z + 5.0) / 10.0;
    
    // Bounds check with slight margin
    if (texCoord.x < 0.01 || texCoord.x > 0.99 || 
        texCoord.y < 0.01 || texCoord.y > 0.99 || 
        texCoord.z < 0.01 || texCoord.z > 0.99) {
        discard;
    }

    vec3 v = texture3D(fluidTex, texCoord).rgb;
    float mag = length(v);
    
    // Increase sensitivity: even 10% of speed shows color
    float t = mag / (speedScale + 2.0);
    if (t > 1.0) t = 1.0;

    vec3 heatmap;
    if (t < 0.15) heatmap = vec3(0.0, 0.0, 0.5 + 3.33*t); // Dark Blue -> Blue
    else if (t < 0.4) heatmap = vec3(0.0, 4.0*(t-0.15), 1.0); // Blue -> Cyan
    else if (t < 0.65) heatmap = vec3(0.0, 1.0, 1.0 - 4.0*(t-0.4)); // Cyan -> Green
    else if (t < 0.85) heatmap = vec3(4.0*(t-0.65), 1.0, 0.0); // Green -> Yellow
    else heatmap = vec3(1.0, 1.0 - 5.0*(t-0.85), 0.0); // Yellow -> Red
    
    gl_FragColor = vec4(heatmap, 0.7);
}