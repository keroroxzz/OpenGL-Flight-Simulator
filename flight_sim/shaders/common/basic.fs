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

float noise3d(vec3 p) {

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

    return l(l(v,vr,ft.x), l(vd,vo,ft.x),ft.y);
}

float clamp(float v, float min_, float max_)
{
    return min(max(v, min_), max_);
}

vec2 clamp(vec2 v, float min, float max)
{
    return vec2(clamp(v.x, min, max), clamp(v.y, min, max));
}

vec3 clamp(vec3 v, float min, float max)
{
    return vec3(clamp(v.x, min, max), clamp(v.y, min, max), clamp(v.z, min, max));
}