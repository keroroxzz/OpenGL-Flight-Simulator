#version 400 core

in vec2 TexCoord;
in vec3 wNorm;
out vec4 FragColor;

uniform int windowWidth;
uniform int windowHeight;
uniform mat4 InvVP;
uniform vec4 camera;
uniform vec3 SunDirection;
uniform sampler2D atmosphere_texture;

uniform mat4 ModelViewProjectionMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 ModelViewMatrix;
uniform mat3 NormalMatrix;

float atmosphereStep = 25.0;
float fix=0.0000001;

vec3 sunLightStrength = 685.0*vec3(1.0, 0.96, 0.949);

float rayleighStrength = 20.0;
float rayleighDecay = 13000.0;

float mieStrength = 50.0;
float mieDecay = 9000.0;

// vec3 waveLengthFactor = vec3(1785.0625, 850.3056, 450.0625);
vec3 waveLengthFactor = vec3(1585.0625, 800.3056, 470.0625);
vec3 scatteringFactor = waveLengthFactor/rayleighDecay;


// const float atmosphereRadius = 5.0;
// const float groundHeight = 3.5;
const float atmosphereRadius = 6.421;
const float groundHeight = 6.371;
const vec3 atmOrigin = vec3(0.0, 0.0, -groundHeight);
const vec4 groundSphere = vec4(0.0, 0.0, 0.0, groundHeight);
const vec4 atmosphereSphere = vec4(0.0, 0.0, 0.0, atmosphereRadius);
const float game2atm = 10000000.0;

float moonReflecticity = 0.16;
float starStrength = 0.125;


float hash(float n) {
    return fract(sin(n/1873.1873) * 1093.0);
}

float l(float a, float b, float x){
    return a*(1.0-x)+b*x;
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

void sphereCast(vec3 origin, vec3 ray, vec4 sphere, out vec3 start, out vec3 end)
{
    vec3 p = origin - sphere.xyz;
    
    float 
        r = length(p),
        d = length(cross(p, ray));    //nearest distance from view-ray to the center
        
    if (d > sphere.w+fix) {
        start = vec3(0.0);
        end = vec3(0.0);
        return;    //early exit
    }
        
    float
        sr = sqrt(sphere.w*sphere.w - d*d),    //radius of slicing circle
        dr = -dot(p, ray);    //distance from camera to the nearest point\
    
    if(r>sphere.w && dr>0){
        start = origin + ray * (dr-sr);
        end = origin + ray * (dr+sr);
    }else if(r<=sphere.w){
        start = origin;
        end = origin + ray * (dr+sr);
    }else{
        start = vec3(0.0);
        end = vec3(0.0);
    }
}

void rayDistance(vec3 posCam, vec3 ray, out vec3 start, out vec3 end){
    vec3 startAtm, endAtm;
    vec3 startGnd, endGnd;
    sphereCast(posCam, ray, atmosphereSphere, startAtm, endAtm);
    sphereCast(posCam, ray, groundSphere, startGnd, endGnd);

    if (startAtm==posCam){
        start = posCam;
        if (startGnd==vec3(0.0))
            end = endAtm;
        else
            end = startGnd;
    }
    else{
        start = startAtm;
        if (startGnd==vec3(0.0))
            end = endAtm;
        else
            end = startGnd;
    }
}

vec3 rayleighScattering(float c)
{
    return (1.0+c*c)*rayleighStrength/waveLengthFactor;
}

float MiePhase(float c)
{
    return 1.0f+1.6f*exp(20.0f*(c-1.0f));
}

float MieScattering(float c)
{
    return mieStrength*MiePhase(c);
}

vec3 LightDecay(float densityR, float densityM)
{
    // why do we need to clamp here?
    return clamp(vec3(exp(-densityR/scatteringFactor - densityM*mieDecay)), 0.0, 1.0);
}

vec4 getVerticalVecHeight(dvec3 atmpos)
{
    double r = length(atmpos);
    double height = (r-groundHeight)/(atmosphereRadius-groundHeight);
    return vec4(atmpos/r, height);
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

vec2 sampleAtmosphere(dvec3 atmpos, vec3 dir)
{
    dvec4 atmHeight = getVerticalVecHeight(atmpos);
    vec2 texcoord = vec2(pow(float(atmHeight.w), 0.5), dot(atmHeight.xyz, dir)*0.5+0.5);
    return texture(atmosphere_texture, texcoord).xy;
}

vec3 AtmosphereScattering(vec3 background, vec3 camPos, vec3 ray, vec3 lightStrength, vec3 lightDirection)
{
    vec3 intensity=vec3(0.0);
    
    float ang_cos = dot(ray, lightDirection);
    float mie = MieScattering(ang_cos);
    vec3 raylei = rayleighScattering(ang_cos);
    
    vec3 start, end;
    rayDistance(camPos, ray, start, end);

    if (start==vec3(0.0))
        return background;

    vec3 marchPos = start;
    vec3 marchStep = (end - start)/atmosphereStep;

    vec2 lastViewToPoint = vec2(0.0);
    vec2 throughShell = sampleAtmosphere(start, ray);

    for(float i=0.0; i<atmosphereStep; i++)
    {
        vec2 viewToPoint = throughShell - sampleAtmosphere(marchPos, ray);
        vec2 pointToSun = sampleAtmosphere(marchPos, lightDirection);
        vec2 viewToSun = (pointToSun + viewToPoint);

        vec2 density = (viewToPoint - lastViewToPoint);

        intensity += LightDecay(viewToSun.x, viewToSun.y)*(raylei*density.x + mie*density.y) + 0.0000001/scatteringFactor*LightDecay(0.0, pointToSun.y);
        lastViewToPoint = viewToPoint;

        marchPos+=marchStep.xyz;
    }
    return lightStrength*intensity + background*LightDecay(throughShell.x, throughShell.y) + 0.004/scatteringFactor*LightDecay(0.0, throughShell.y);
}

vec3 Game2Atm(vec3 pos)
{
    return pos/game2atm-atmOrigin;
}

void main(void) {
    vec3 direction = normalize(wNorm);
    vec3 sun = sunLightStrength*vec3(smoothstep(0.9999, 1.0, dot(SunDirection,direction)));
    vec3 c = AtmosphereScattering(sun, Game2Atm(camera.xyz), direction, sunLightStrength, SunDirection);
    
    FragColor = vec4(c, 1.0);
}
