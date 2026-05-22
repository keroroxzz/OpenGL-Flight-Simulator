#version 120
varying vec3 gridPos;
uniform mat4 sliceMatrix;
void main() {
    // Transform vertex from Slice-Local to Aircraft-Local Space
    gridPos = (sliceMatrix * gl_Vertex).xyz;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}