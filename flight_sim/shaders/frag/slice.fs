#version 120
uniform sampler3D fluidTex;
uniform float speedScale;
varying vec3 gridPos;

void main() {
    // Grid spans: X: -10 to 20, Y: -15 to 15, Z: -15 to 15
    vec3 texCoord;
    texCoord.x = (gridPos.x + 10.0) / 30.0;
    texCoord.y = (gridPos.y + 15.0) / 30.0;
    texCoord.z = (gridPos.z + 15.0) / 30.0;
    
    // Bounds check with slight margin
    if (texCoord.x < 0.01 || texCoord.x > 0.99 || 
        texCoord.y < 0.01 || texCoord.y > 0.99 || 
        texCoord.z < 0.01 || texCoord.z > 0.99) {
        discard;
    }

    vec3 v = texture3D(fluidTex, texCoord).rgb;
    float mag = length(v);
    
    // Normalize heatmap intensity
    float t = mag / (speedScale + 5.0);
    if (t > 1.0) t = 1.0;

    vec3 heatmap;
    if (t < 0.25) heatmap = vec3(0.0, 4.0*t, 1.0);
    else if (t < 0.5) heatmap = vec3(0.0, 1.0, 1.0 - 4.0*(t-0.25));
    else if (t < 0.75) heatmap = vec3(4.0*(t-0.5), 1.0, 0.0);
    else heatmap = vec3(1.0, 1.0 - 4.0*(t-0.75), 0.0);
    
    gl_FragColor = vec4(heatmap, 0.7);
}