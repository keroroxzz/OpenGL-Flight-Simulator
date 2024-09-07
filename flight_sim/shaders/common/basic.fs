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

float worley(vec3 p, float seed)
{
    vec3 pi = floor(p);
    vec3 pf = fract(p);
    vec3 d = vec3(8.0);
    for (int i = -1; i <= 1; i++)
    {
        for (int j = -1; j <= 1; j++)
        {
            for (int k = -1; k <= 1; k++)
            {
                vec3 o = vec3(float(i), float(j), float(k));
                vec3 r = vec3(hash(seed + dot(pi + o, vec3(127.1, 311.7, 415.9))));
                vec3 p = o + r - pf;
                float d1 = dot(p, p);
                float d2 = dot(1.0 - p, 1.0 - p);
                d = min(d, vec3(d1, d2, d1 + d2));
            }
        }
    }
    return sqrt(d.x);
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