/*
    GTA SA CloudWorks by Brian Tu 
    Version : GLSL Alpha (3.7.1)
    Date : 2021/2/7
    Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License
    Please cite the author if you use this code in your work.
    
    Note : haven't optimized the code yet
*/


#version 330 core
in vec4 position;  // position of the vertex (and fragment) in world space
in vec3 wNorm;  // surface normal vector in world space
in vec3 vNorm;
in vec2 TexCoord;

out vec4 FragColor;

uniform int windowWidth;
uniform int windowHeight;
uniform mat4 InvVP;
uniform vec4 camera;
uniform vec3 SunDirection;
uniform sampler2D sampler0;

uniform mat4 ShadowMVP;
uniform sampler2D sampler1;

vec4 scene_ambient = vec4(1.0, 1.0, 1.1, 1.0) * (smoothstep(-0.1, 0.3, SunDirection.z) + 0.3*smoothstep(-0.2, -0.3, SunDirection.z));


//========================================================================

vec3 skybot = vec3(200, 210, 225) / 255.0;
vec3 skytop = vec3(70, 110, 155) / 255.0;
vec3 skybot2 = vec3(170, 150, 150) / 255.0;
vec3 skytop2 = vec3(80, 100, 135) / 250.0;

#define float3 vec3
#define float2 vec2
#define float4 vec4
#define lerp mix
#define frac fract
#define atan2 atan
const float3 vGradingColor0=float3(0.0,0.0,0.0);
const float3 vGradingColor1=float3(0.7,0.5,0.7);
const float3 vHorizonCol=float3(1.0,1.0,1.0);
const float fFogStart=0.0;
const float CloudStartHeight=0.0;
const float CloudEndHeight=0.0;

#define ATM_DENSE vGradingColor0.r*7.0f
#define MIST vGradingColor0.g

float atmosphereStep = 15.0;
float lightStep = 3.0;
float fix=0.00001;

float3 sunLightStrength = 685.0*float3(1.0, 0.96, 0.949);
float moonReflecticity = 0.16;
float starStrength = 0.75;
float LightingDecay = 300.0;

float rayleighStrength = 1.85;
const float rayleighDecay = 900.0;
float atmDensity = 3.0 + ATM_DENSE;
float atmCondensitivity = 1.0;

const float3 waveLengthFactor = float3(1785.0625, 850.3056, 410.0625);
const float3 scatteringFactor = waveLengthFactor/rayleighDecay;   // for tuning, should be 1.0 physically, but you know.

float mieStrength = 0.125 + MIST*0.1;
float mieDecay = 100.0;
float fogDensity = 0.25 + MIST;

const float earthRadius = 6.421;
const float groundHeight = 6.371;
const float3 AtmOrigin = float3(0.0, 0.0, groundHeight);

float4 earth = float4(0.0, 0.0, 0.0, earthRadius);

float game2atm = 1000000.0;

//=====Basic functions=====
float saturate(float v)
{
    return min(max(v,0.0),1.0);
}

float3 saturate(float3 v)
{
    return float3(saturate(v.x),saturate(v.y),saturate(v.z));
}

float l(float a, float b, float x){
    return a*(1.0-x)+b*x;
}

float3 Game2Atm(float3 gamepos)
{
    return gamepos/game2atm + AtmOrigin;
}

float3 Game2Atm_Alt(float3 gamepos)
{
    return float3(0.0, 0.0, gamepos.z/game2atm) + AtmOrigin;
}


//============IQ's noise===================
//This noise generator is developed by Inigo Quilez.
//License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License


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
//=========================================
float noise2d(vec2 p) {

    float2 fr = floor(p),
    ft = frac(p);

    float n = 1153.0 * fr.x + 2381.0 * fr.y,
    nr = n + 1153.0,
    nd = n + 2381.0,
    no = nr + 2381.0,

    v = hash(n),
    vr = hash(nr),
    vd = hash(nd),
    vo = hash(no);

    return ((v * (1.0 - ft.x) + vr * (ft.x)) * (1.0 - ft.y) + (vd * (1.0 - ft.x) + vo * ft.x) * ft.y);
}

float noise3d(float3 p) {

    float3 fr = floor(p),
    ft = frac(p);

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

float noise3d_timedep(float3 p, float time) {

    float3 fr = floor(p),
    ft = frac(p);

    float n = 1153.0 * fr.x + 2381.0 * fr.y + fr.z + time,
    nr = n + 1153.0,
    nd = n + 2381.0,
    no = nr + 2381.0,

    v = l(hash(n), hash(n + 1.0), ft.z),
    vr = l(hash(nr), hash(nr + 1.0), ft.z),
    vd = l(hash(nd), hash(nd + 1.0), ft.z),
    vo = l(hash(no), hash(no + 1.0), ft.z);

    return l(l(v,vr,ft.x), l(vd,vo,ft.x),ft.y);
}

float map(float x, float a, float b, float c, float d) {
    return (x - b) / (a - b) * (c - d) + d;
}


//=====Cast functions=====
float4 sphereCast(float3 origin, float3 ray, float4 sphere, float step, out float3 begin)
{
    float3 p = origin - sphere.xyz;
    
    float 
        r = length(p),
        d = length(cross(p, ray));    //nearest distance from view-ray to the center
        
    if (d > sphere.w+fix) return float4(0.0);    //early exit
        
    float
        sr = sqrt(sphere.w*sphere.w - d*d),    //radius of slicing circle
        dr = -dot(p, ray);    //distance from camera to the nearest point
        
    float3
        pc = origin + ray * dr,
        pf = pc + ray * sr,
        pb = pc - ray * sr;
    
    float sl; //stpe length
    
    if(r > sphere.w){
        
        begin = pb;
        sl = sr*2.0/step;
    }
    else{
    
        begin = origin;
        sl = length(pf - origin)/step;
    }
    return float4(ray * sl, sl);
}

//=====Atomosphere scattering functions=====
float2 Density(float3 pos, float4 sphere, float strength, float condense)
{
    float r = groundHeight;
    float h = length(pos-sphere.xyz)-r;
    float ep = exp(-(sphere.w-r)*condense);
    
    float fog = fogDensity*clamp((1.0/(1200.0*h+0.5)-0.04)/1.96,0.0,1.0);
    
    if(h<0.0)
        return float2(strength, fogDensity);

    return float2((exp(-h*condense)-ep)/(1.0-ep)*strength, fog);
}

float3 rayleighScattering(float c)
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

float3 LightDecay(float densityR, float densityM)
{
    return float3(exp(-densityR/scatteringFactor - densityM*mieDecay));
}

//=====Sun light functions=====
float3 SunLight(float3 light, float3 position, float3 lightDirection, float4 sphere)
{
    float3 smp;
    float4 sms = sphereCast(position, lightDirection, sphere, lightStep, smp);
        
    float2 dl;
    
    for(float j=0.0; j<lightStep; j++)
    {
        smp+=sms.xyz/2.0;
        dl += Density(smp, sphere, atmDensity, atmCondensitivity)*sms.w;
        smp+=sms.xyz/2.0;
    }
    
    return light*LightDecay(dl.x, dl.y)/LightingDecay;
}

float3 LightSource(float3 SunDirection, out float3 SourceDir)
{
    if(SunDirection.z<-0.075)
    {
        SourceDir = -SunDirection;
        return sunLightStrength*moonReflecticity*smoothstep(-0.075, -0.105, SunDirection.z);
    }
    
    SourceDir = SunDirection;
    return sunLightStrength*smoothstep(-0.075, -0.025, SunDirection.z);
}

//=====Atmosphere functions=====
vec4 atomosphere(float depth, float pz, float vz)
{
    float h0 = 0.0, 
        h1 = 1300.0,
        a = 700.0,
        density = 0.0002;

    float t, c1, e1, e2, e3, res;
    
    //Fix for altitude higher than h1
    if (pz>h1)
    {
        depth = max(0.0, depth + (pz - h1) / vz);
        pz = h1;
    }
    
    //calculate the integral distance
    t = min(depth, (vz>=0.0) ? (h1 - pz)/vz : 99999999.0);
        
    //constants and values
    c1 = a/vz;
    e1 = exp((h0 - pz)/a);
    e2 = exp((h0 - pz - t*vz)/a);
    e3 = exp((h0 - h1)/a);
    
    //integral
    res = (c1*e1 - c1*e2 - e3*t)*density;
    res = 1.0-exp(-res);
        
    return vec4(skybot, res*res);
}
  
float3 AtmosphereScattering(float3 background ,float3 marchPos, float4 marchStep, float3 ray, float3 lightStrength, float3 lightDirection, float strength, float4 sphere)
{
    float3 intensity=float3(0.0);
    
    float ang_cos = dot(ray, lightDirection);
    float mie = MieScattering(ang_cos);
    float3 raylei = rayleighScattering(ang_cos);
    
    if(marchStep.w>0.015)
        marchStep /= marchStep.w/0.015;
    
    float2 dv=float2(0.0);
    for(float i=0.0; i<atmosphereStep; i++)
    {
        float3 smp;
        float4 sms = sphereCast(marchPos, lightDirection, sphere, lightStep, smp);
        float2 sampling = Density(marchPos, sphere, atmDensity, atmCondensitivity)*marchStep.w;
            
        dv += sampling/2.0;
        
        float2 dl=dv, dd;
        for(float j=0.0; j<lightStep; j++)
        {
            smp+=sms.xyz;
            float2 test = Density(smp, sphere, atmDensity, atmCondensitivity)*sms.w;
            dl += test;
            dd += test;
        }
        
        intensity += LightDecay(dl.x, dl.y)*(raylei*sampling.x + mie*sampling.y)+raylei*LightDecay(0.0, dd.x*0.0001)*0.002;
        
        dv += sampling/2.0;

        //marching
        marchPos+=marchStep.xyz;
    }
    
    return lightStrength*intensity*strength + background*LightDecay(dv.x, dv.y);
}

float3 ExponentialFog(float3 camera, float3 viewDir, float distance, out float transparency)
{
    float h0 = 0.0, 
        h1 = 160.0,
        a = 38.0,
        density = fFogStart/2000.0 + 0.00000001f;   //plus a small value to fix a weird problem

    float t, c1, e1, e2, e3, res;
    
    //fix horizontal error
    if(abs(viewDir.z)<=0.0001)
        viewDir.z=viewDir.z>0.0?0.0001:-0.0001;
    
    //Fix for altitude higher than h1
    if (camera.z>h1)
    {
        distance = max(0.0, distance + (camera.z - h1) / viewDir.z);
        camera.z = h1;
    }
    
    //ray length (inverse)
    t = max(1.0/distance, (viewDir.z>0.0) ? viewDir.z/(h1 - camera.z) : 0.0);
        
    //constants and values
    c1 = viewDir.z/a;
    e1 = exp((h0 - camera.z)/a);
    e2 = exp((h0 - camera.z - viewDir.z/t)/a);
    e3 = exp((h0 - h1)/a);
    
    //calculate the exp fog
    res = (e1 - e2)/c1 - e3/t;
    
    transparency = exp(-res*density);
    return (1.0f-transparency)*vHorizonCol;
}

float3 atmosphere_scattering(float strength, float3 color, float3 camera, float3 ray, float distance, float3 SunDirection, float4 sphere)
{
    //fade-in and cut off
    if(distance<200.0)
        return color;
    float fade=smoothstep(200.0, 300.0, distance);
    
    //additionally multiply the distance since the map scale is too small to be scattered.
    float4 marchStep=float4(0.0);
    marchStep.w = 15.0*distance/atmosphereStep/game2atm;
    marchStep.xyz = ray*marchStep.w;
    
    float3 lightDirection = float3(0.0);
    float3 lightStrength = LightSource(SunDirection, lightDirection);
    
    float3 scattered = AtmosphereScattering(color,  Game2Atm_Alt(camera), marchStep, ray, lightStrength, lightDirection, 1.0, sphere);
    return lerp(color, scattered, fade);
}

float3 OutterSpace(float3 ray, float3 SunDirection)
{
    float3 res=float3(0.0);

    float ang_cos = dot(ray, SunDirection);
    
    //The sun
    if(ang_cos>0.9999){
        res += 0.1*sunLightStrength*smoothstep(0.9999,0.99995,ang_cos);
    }
        
    //The moon
    else if(ang_cos<-0.99978){
        res += 0.03*moonReflecticity*sunLightStrength*smoothstep(-0.99978,-0.99985,ang_cos);
    }
    
    return res;
}

float3 RadientLight(float3 sunDirection, float3 ray)
{
    float strength = smoothstep(0.1, 0.0,  sunDirection.z) * smoothstep(-0.25, 0.0,  sunDirection.z);
    strength *= (pow((ray.z+1.0)/2.0, 3.0)*0.5 + 0.5*dot(ray, sunDirection)+0.5);
    
    return float3(0.13, 0.15, 0.15)*saturate(strength);
}

float3 Sky(float3 background, float3 camera, float3 ray, float3 SunDirection, float4 sphere)
{
    float3 marchPos;
    float4 marchStep = sphereCast(camera, ray, sphere, atmosphereStep, marchPos);
    
    if (marchStep.w == 0.0) return background;
    
    float strength=1.0;
    float3 ground;
    if(sphereCast(camera, ray, float4(0.0, 0.0, 0.0, groundHeight-0.00075), atmosphereStep, ground).w>0.0)
    {
        if(length(ground-marchPos)>0.0 && dot(ground-marchPos, ray)>0.0)
        {
            marchStep.xyz = (ground-marchPos)/atmosphereStep;
            marchStep.w = length(marchStep.xyz);
            // background=float3(0.0);
        }
    }
    
    float3 lightDirection = float3(0.0);
    float3 lightStrength = LightSource(SunDirection, lightDirection);
    
    float3 radientFix = RadientLight(SunDirection, ray);
    
    return AtmosphereScattering(background, marchPos, marchStep, ray, lightStrength, lightDirection, strength, sphere) + radientFix;
}

//========================================================================

void main()
{  
  // basics
  vec3 normalDirection = normalize(wNorm);
  float depth = gl_FragCoord.z / gl_FragCoord.w;

  // world coord
  vec4 ndc = vec4(gl_FragCoord.x / windowWidth * 2.0 - 1.0, gl_FragCoord.y / windowHeight * 2.0 - 1.0, gl_FragCoord.z * 2.0 - 1.0, 1.0);
  vec4 ec = InvVP * ndc;
  vec4 worldPosition = ec / ec.w;
  vec3 viewRay = normalize(worldPosition.xyz- camera.xyz);

  // shadow map
  vec4 sp = ShadowMVP*worldPosition;
  sp = sp/sp.w;
  sp.xyz = (sp.xyz+1.0)/2.0;
  float d = texture(sampler1, sp.xy).r;
  float non_shaded = smoothstep(0.01, 0.0, sp.z-d) * smoothstep(0.1, 0.5, dot(normalDirection, SunDirection));

  // FragColor = vec4(vec3(noise2d(TexCoord*1000.0)), 1.0);
  // return;
  // Sky reflection
  vec3 reflectedRay;
  vec3 skyLight;
  
  int max = 10;
  for (int i = 0; i < max; i++){
  reflectedRay = reflect(viewRay, normalize(normalDirection+vec3(noise2d(TexCoord*9000.145960-135*i), noise2d(TexCoord*9000.145960-5464.541*i), noise2d(TexCoord*7568.156465+50.156*i))*0.1));
  skyLight += Sky(OutterSpace(reflectedRay, SunDirection), Game2Atm_Alt(camera.xyz), reflectedRay, SunDirection, earth);
  }
  skyLight *= 0.15*(1.0 - pow(dot(normalDirection, reflectedRay), 3.0))/max;

  // Lighting
  vec3 lightDirection = vec3(0.0);
  vec3 sunLightSource = LightSource(SunDirection, lightDirection);
  vec3 ambientLighting = Sky(vec3(0.0), Game2Atm_Alt(camera.xyz), vec3(0.0,0.0,1.0), SunDirection, earth);
  vec3 light = 0.8*SunLight(sunLightSource, Game2Atm_Alt(worldPosition.xyz), lightDirection, earth);
  
  // float illumination = (dot(normalDirection, lightDirection)-0.4)*light_str/0.6,
  // smoothIllumination = max(0.0, illumination) * smoothstep(0.0, 0.05, illumination);
  
  // specularReflection = specularReflection * smoothIllumination*light_str;
    
  // float vnDot=dot(normalDirection, viewRay.xzy), fakefrensel = min(1.0/abs(vnDot), 1.0);
  // vec3 reflectionRay = viewRay.xzy - 2*vnDot*normalDirection;
  // float rfDot = dot(reflectionRay, lightDirection),
  // reflectiveLight = smoothstep(0.6, 1.0, rfDot)*fakefrensel*light_str;
  
  vec3 color = (ambientLighting + light*non_shaded)*texture(sampler0, TexCoord).xyz;
  FragColor = vec4(color + skyLight, 1.0);
}



















