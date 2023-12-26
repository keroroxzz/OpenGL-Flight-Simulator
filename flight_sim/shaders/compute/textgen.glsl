#version 430 core
layout (local_size_x=1, local_size_y=1, local_size_z=1) in;

layout (r16f, binding = 0) uniform image3D input_image;
layout (r16f, binding = 1) uniform image3D output_image;

uniform float time;

float saturate(float v)
{
    return min(max(v,0.0),1.0);
}

vec3 saturate(vec3 v)
{
    return vec3(saturate(v.x),saturate(v.y),saturate(v.z));
}

float l(float a, float b, float x){
    return a*(1.0-x)+b*x;
}


float hash(float n) {
    return fract(sin(n/1873.1873) * 1093.0);
}

float noise_p(vec3 p) {

    vec3 fr = floor(p),
    ft = fract(p);

    float n = 1153.0 * fr.x + 2381.0 * fr.y + fr.z,
    nr = n + 1153.0,
    nd = n + 2381.0,
    no = nr + 2381.0,

    v = l(hash(n), hash(n + 1.0), ft.z),
    vr = l(hash(nr), hash(nr + 1.0), ft.z),
    vd = l(hash(nd), hash(nd + 1.0), ft.z),
    vo = l(hash(no), hash(no + 1.0), ft.z);

    return ((v * (1.0 - ft.x) + vr * (ft.x)) * (1.0 - ft.y) + (vd * (1.0 - ft.x) + vo * ft.x) * ft.y);
}

void main(void)
{
    vec3 pos = 2.0*vec3(gl_GlobalInvocationID.xyz) / vec3(gl_NumWorkGroups.xyz*gl_WorkGroupSize.xyz);

    float seed = 100.0*int(time/3.0);
    float current = imageLoad(input_image, ivec3(gl_GlobalInvocationID.xyz)).x;
    float next = noise_p(pos+seed);

    imageStore(output_image, ivec3(gl_GlobalInvocationID.xyz), vec4((next*0.005+current*0.995)));
}