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
uniform float iTime;
uniform mat4 InvVP;
uniform vec4 camera;
uniform vec3 SunDirection;
uniform sampler2D sampler0;
uniform sampler3D test;
uniform sampler3D atmText;

uniform mat4 ModelViewProjectionMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 ModelViewMatrix;
uniform mat3 NormalMatrix;

#define lerp mix
#define frac fract
#define atan2 atan
const vec3 vGradingColor0=vec3(0.0,0.0,0.0);
const vec3 vGradingColor1=vec3(0.7,0.5,0.7);
const vec3 vHorizonCol=vec3(1.0,1.0,1.0);
const float fFogStart=0.0;
const float CloudStartHeight=0.0;
const float CloudEndHeight=0.0;

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

//============IQ's noise===================
//This noise generator is developed by Inigo Quilez.
//License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License


float hash(float n) {
    return fract(sin(n/1873.1873) * 1093.0);
}

float noise_p(vec3 p) {

    vec3 text3d = texture(
        test, p).xyz;

    return text3d.x;
}
//=========================================

float noise3d(vec3 p) {

    vec3 fr = floor(p),
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

float map(float x, float a, float b, float c, float d) {
    return (x - b) / (a - b) * (c - d) + d;
}

#define ATM_DENSE vGradingColor0.r*7.0f
#define MIST vGradingColor0.g

float atmosphereStep = 100.0;
float lightStep = 4.0;
float fix=0.00001;

vec3 sunLightStrength = 685.0*vec3(1.0, 0.96, 0.949);
float moonReflecticity = 0.16;
float starStrength = 0.75;
float LightingDecay = 300.0;

vec2 atmDensity = vec2(1.0, 0.015);
float rayleighStrength = 1.0;
const float rayleighDecay = 300.0;

const vec3 waveLengthFactor = vec3(1785.0625, 850.3056, 410.0625);
const vec3 scatteringFactor = waveLengthFactor/rayleighDecay;   // for tuning, should be 1.0 physically, but you know.

float mieStrength = 10.06;
float mieDecay = 0.0;

vec4 sphereCast(vec3 origin, vec3 ray, vec4 sphere, float step, out vec3 begin)
{
    vec3 p = origin - sphere.xyz;
    
    float 
        r = length(p),
        d = length(cross(p, ray));    //nearest distance from view-ray to the center
        
    if (d > sphere.w+fix) return vec4(0.0);    //early exit
        
    float
        sr = sqrt(sphere.w*sphere.w - d*d),    //radius of slicing circle
        dr = -dot(p, ray);    //distance from camera to the nearest point
        
    vec3
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
    return vec4(ray * sl, sl);
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
    return vec3(exp(-densityR/scatteringFactor - densityM*mieDecay));
}

vec3 SunLight(vec3 light, vec3 position, vec3 lightDirection, vec4 sphere)
{
    vec2 dl = atmDensity*texture(
        atmText, 
        vec3(position.z, atan2(length(lightDirection.xy), lightDirection.z)/3.14159-0.5, 1.0)).xy;
    
    return light*LightDecay(dl.x, dl.y)/LightingDecay;
}

vec3 LightSource(vec3 SunDirection, out vec3 SourceDir)
{
    if(SunDirection.z<-0.075)
    {
        SourceDir = -SunDirection;
        return sunLightStrength*moonReflecticity*smoothstep(-0.075, -0.105, SunDirection.z);
    }
    
    SourceDir = SunDirection;
    return sunLightStrength*smoothstep(-0.075, -0.025, SunDirection.z);
}


vec3 skybot = vec3(200, 210, 225) / 255.0;
vec3 skytop = vec3(70, 110, 155) / 255.0;
vec3 skybot2 = vec3(170, 150, 150) / 255.0;
vec3 skytop2 = vec3(80, 100, 135) / 250.0;

float vb_t  = 1000.0;
float vb_b  = 0.0;
float min_sl  = 5.0;
float max_sl  = 50.0;
float step_mul  = 5.0;
float maxStep  = 150.0;
float dCut  = 0.1;
float tCut  = 0.1;

float body_top  = 1500.0;
float body_mid  = 150.0;
float body_bot  = 0.0;
float body_thickness  = 0.0;

float grow  = 0.87;
float grow_c  = 0.55;
float soft_top  = 0.9;
float soft_bot  = 0.8;
float soft_bot_c  = 1.5;

float noise_densA  = 0.40;
float noise_densB  = 0.20;
float noise_densC  = 0.30;
float noise_densD  = 0.20;
float noise_densE  = 0.50;
float noiseSizeA  = 5000.0;
float noiseSizeB  = 900.0;
float noiseSizeC  = 600.0;
float noiseSizeD  = 200.0;
float noiseSizeE  = 75.0;

vec3 cloud_shift  = vec3(-1.0, 0, 0);
float curvy_ang  = 5.0;
float curvy_strength  = 100.0;

float BeerFactor  = 1.5;
float EffectFactor  = 1.0;
float solidness_top = 0.7;
float solidness_bot = 0.2;
float scattering  = 0.0;
float cloudBrightnessMultiply  = 0.6;

float shadowStep = 20.0;
float ShadowStepLength  = 30.0;

float alpha_curve  = 1.0;
float fade_s  = 0.0;
float fade_e  = 10000.0;

float speed  = 300.0;
vec3 nA_move  = vec3(-1, 2, 0.29);
vec3 nB_move  = vec3(-4, 1.8, 0.0);
vec3 nC_move  = vec3(-3.5, 1.5, 1.0);
vec3 nD_move  = vec3(-3.0, 1, -0.2);

vec3 CBC_D  = vec3(160, 168, 180)/255.0*0.8;
vec3 CBC_N  = vec3(0.118, 0.118, 0.118)*0.5;
vec3 CBC_S  = vec3(0.45, 0.45, 0.48)*0.5;
vec3 CBC_CD  = vec3(0.557, 0.616, 0.659);
vec3 CBC_CN  = vec3(0.275, 0.275, 0.275);
vec3 CTC_D  = vec3(255, 248, 250)/255.0;
vec3 CTC_N  = vec3(0.153, 0.125, 0.0824)*0.7;
vec3 CTC_S  = vec3(1.0, 0.7, 0.56);
vec3 CTC_CD  = vec3(0.894, 0.949, 0.957);
vec3 CTC_CN  = vec3(0.125, 0.157, 0.173);

#define nsA		0.04/noiseSizeA
#define nsB		0.04/noiseSizeB
#define nsC		0.1/noiseSizeC
#define nsD		0.1/noiseSizeD
#define nsE		0.1/noiseSizeE

vec3 PosOnPlane(const in vec3 origin, const in vec3 direction, const in float h, inout float dis) {
    dis = (h - origin.z) / direction.z;
    return origin + direction * dis;
}

float clampMap(float x, float a, float b, float c, float d) {
    return clamp((x - b) / (a - b), 0.0, 1.0) * (c - d) + d;
}

//generate parameters
vec4 cloud_shape(float z, vec4 profile, vec3 dens_thres) {
    float soft = clampMap(z, profile.y, profile.x, dens_thres.z, dens_thres.y); //range of the soft part
    float solidness = clampMap(z, profile.y, profile.x, solidness_bot, solidness_top); //range of the soft part
    return
    vec4(
        smoothstep(profile.z, l(profile.y, profile.z, profile.w), z) * smoothstep(profile.x, l(profile.y, profile.x, profile.w), z),    //shape factor
        dens_thres.x + soft,
        dens_thres.x - soft,
        solidness);
}


float noise_d(const in vec3 p) {
    return noise_p(p);
}

//large chunk for the distribution of clouds.
float chunk(in vec3 pos, const in vec3 offsetA, const in vec3 offsetB, in float cs) {

    pos += cloud_shift * pos.z;
    
    vec3
        pA = pos.xyz + vec3(offsetA.xy, 0.0),
        pB = pos.xyz + vec3(offsetB.xy, 0.0);
    
    float dens_a = noise_densA * noise_d(pA * nsA);
    
    return clamp((dens_a + noise_densB * noise_d(pB * nsB)) * cs, 0.0, 1.0);
}

//Detial noise for clouds
float detial( const in vec3 pos, const in vec3 offsetC, const in vec3 offsetD) {
    vec3
        pC = pos + offsetC,
        pD = pos + offsetD;

    
    float nc = noise_p(pC * nsC);

    //curvy distortion
    float a = nc * curvy_ang;
    vec3 d = vec3(cos(a), sin(a), 0.0) * curvy_strength;

    //#define A

    #ifdef A
    float dens = noise_densC * nc + noise_densD * (1.0 - noise_p((pD + d) * nsD)) + noise_densE * noise_p((pD) * nsE);
    #endif
    
    #ifndef A
    float dens = noise_densC * nc + noise_densD * (1.0 - noise_p((pD + d/3.0) * nsD));
    dens = dens * (noise_densE * noise_p((pD + d) * nsE)+1.0);
    #endif

    return dens;
}

float detial2(const in vec3 pos, const in vec3 offsetC) {
    vec3
    pC = pos + offsetC;

    return noise_densC * noise_p(pC * nsC) * 2.0;
}

float brightness(vec3 p, vec4 profile, vec3 dens_thres, const in vec3 offsetA, const in vec3 offsetB, const in vec3 offsetC) {

    float v=0.0;

    for(float r=0.5;r<shadowStep+0.5;r++)
    {
        vec3 p1 = p + SunDirection.xyz * ShadowStepLength * r;

        if (p1.z > vb_t || p1.z < vb_b)
            break;

        vec4 cs = cloud_shape(p1.z, profile, dens_thres);

        float
            d1 = chunk(p1, offsetA, offsetB, cs.x),
            d2 = detial2(p1, offsetC);
        
        float d = clamp(d1*(d2+1.0)+d2/5.0-0.1, 0.0, 1.0);
        v+=clampMap(d, cs.z, cs.y, 0.0, 1.0) * cs.z * shadowStep;
    }
    return exp(-v / BeerFactor);
}

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

const float earthRadius = 1.0;
const float groundHeight = 0.0;
const vec3 AtmOrigin = vec3(0.0, 0.0, -groundHeight);
const vec4 earth = vec4(0.0, 0.0, 10.0, earthRadius);
const float game2atm = 1.0;

vec3 Game2Atm(vec3 gamepos, vec3 campos)
{
    gamepos.xy -= campos.xy;
    return gamepos/game2atm - AtmOrigin;
}

vec3 Game2Atm_Alt(vec3 gamepos)
{
    return vec3(0.0, 0.0, gamepos.z/game2atm) + AtmOrigin;
}

vec4 getVerticalVecHeight(vec3 atmpos)
{
    vec3 relative = atmpos;
    float height = length(relative);
    return vec4(relative/height, height);
}

vec2 sampleAtmosphere(vec3 atmpos, vec3 dir, float dist)
{
    vec4 atmHeight = getVerticalVecHeight(atmpos);
    return atmDensity*texture(atmText, vec3(atmHeight.w, dot(atmHeight.xyz, dir)*0.5+0.5, dist)).xy;
}

vec3 AtmosphereScattering(vec3 background ,vec3 camPos, vec3 ray, vec3 lightStrength, vec3 lightDirection)
{
    vec3 intensity=vec3(0.0);
    
    float ang_cos = dot(ray, lightDirection);
    float mie = MieScattering(ang_cos);
    vec3 raylei = rayleighScattering(ang_cos);
    
    vec3 marchPos = camPos;
    vec3 atmCamPos = Game2Atm(camPos, camPos);

    vec4 marchStep = sphereCast(atmCamPos, ray, earth, atmosphereStep, marchPos);

    return vec3(noise_p((marchPos + marchStep.xyz*atmosphereStep)/3.0));
    
    // vec2 sampling2 = sampleAtmosphere(atmCamPos, ray, 1.0);
    // return vec3(sampling2.x);

    vec2 dv = vec2(0.0);
    vec2 last = vec2(0.0);
    for(float i=0.0; i<atmosphereStep; i++)
    {
        float dist = length(marchPos-atmCamPos)/earthRadius/2.0;
        vec2 sampling = sampleAtmosphere(atmCamPos, ray, dist);

        float h = length(cross(marchPos, lightDirection));
        // if (h<groundHeight && dot(marchPos, lightDirection)<0.0) continue;
        vec2 test = sampleAtmosphere(marchPos, -lightDirection, 1.0);
        vec2 dl = (test + sampling);
        
        intensity += LightDecay(dl.x, dl.y)*(raylei*(sampling.x-last.x) + mie*(sampling.y-last.y));
        last = sampling;

        marchPos+=marchStep.xyz;
    }
    
    return lightStrength*intensity + vec3(noise_p((marchPos + marchStep.xyz*atmosphereStep)*100.0))*0.5;
}

vec3 ExponentialFog(vec3 camera, vec3 viewDir, float distance, out float transparency)
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

vec3 WorldToSpace(vec3 ray, vec3 SunDirection)
{
    vec3 
        axis_x = normalize(vec3(0.85, -2.0, 0.45)),   //The rotation axis of the Earth
        axis_z = normalize(cross(axis_x, SunDirection)),
        axis_y = normalize(cross(axis_z, axis_x));
        
    return normalize(vec3(dot(axis_x, ray), dot(axis_y, ray), dot(axis_z, ray)));
}

vec3 StarSky(float strength, float density, float curve, vec3 ray)
{
    vec3 star_ray = ray*density;
    
    float
        x = noise3d(star_ray + vec3(250.1654, 161.15, 200.5556)),
        y = noise3d(star_ray + vec3(12.04645, 20.012631, 300.4580)),
        z = noise3d(star_ray - vec3(100.234, -20.0156, 3000.0912));
        
    star_ray = 2.0*vec3(x, y, z)-1.0;
            
    return vec3(pow(saturate(dot(star_ray, ray)), curve)*strength*2.0f);
}

vec3 OutterSpace(vec3 ray, vec3 SunDirection)
{
    vec3 res=vec3(0.0);

    float ang_cos = dot(ray, SunDirection);
    
    //The sun
    if(ang_cos>0.9999){
        res += 0.1*sunLightStrength*smoothstep(0.9999,0.99995,ang_cos);
    }
        
    //The moon
    else if(ang_cos<-0.99978){
        res += 0.03*moonReflecticity*sunLightStrength*smoothstep(-0.99978,-0.99985,ang_cos);
    }
    
    //Stars
    else{
        res += StarSky(starStrength, 300.0, 6.0, WorldToSpace(ray, SunDirection))*smoothstep(0.05, -0.1, SunDirection.z);
    }
    
    return res;
}

vec3 Sky(vec3 background, vec3 camera, vec3 ray, vec3 SunDirection, vec4 sphere)
{
    vec3 lightDirection = vec3(0.0);
    vec3 lightStrength = LightSource(SunDirection, lightDirection);
    
    return AtmosphereScattering(background, camera, ray, lightStrength, lightDirection);
}

vec3 AtmosphereRadient(vec3 marchPos, vec4 marchStep, vec3 ray, vec3 lightStrength, vec3 lightDirection, float strength, vec4 sphere)
{
    vec3 intensity=vec3(0.0);
    
    float ang_cos = dot(ray, lightDirection);
    vec3 raylei = rayleighScattering(ang_cos);
    
    if(marchStep.w>0.015)
        marchStep /= marchStep.w/0.015;
    
    for(float i=0.0; i<atmosphereStep; i++)
    {
        vec3 smp;
        vec4 sms = sphereCast(marchPos, lightDirection, sphere, lightStep, smp);
            
        vec2 dd;
        
        intensity += raylei*LightDecay(0.0, dd.x*0.0001)*0.002;

        //marching
        marchPos+=marchStep.xyz;
    }
    
    return lightStrength*intensity*strength;
}



vec4 Cloud_mid(const in vec3 cam_dir, in vec3 p, in float dist, const in float depth, float time, float moonLight, float tf, float sunny, out float new_depth) {
    vec4 d = vec4(0.0, 0.0, 0.0, min_sl); //distance, distance factor, gap, the last march step

    p = PosOnPlane(p, cam_dir, clamp(p.z, vb_b+0.001, vb_t-0.001), d.x);
    d.y = d.x;

    float suncross = dot(SunDirection.xyz, cam_dir);
    vec3 raylei = rayleighScattering(suncross);
	
    if (d.x >= 0.0) {
        vec3
            flowA = -time * nA_move,
            flowB = -time * nB_move,
            flowC = -time * nC_move,
            flowD = -time * nD_move,

        dens_thres = vec3(
                1.0 / l(grow_c, grow, sunny),
                soft_top,
                l(soft_bot_c, soft_bot, sunny)),

        col = vec3(0.0, 0.0, 0.0),
        fx = vec3(0.0, 0.0, 1.0); //brightness, density, transparency

        vec4 
			profile = vec4(body_top, body_mid, body_bot, body_thickness);

        for (float i = 0.0; fx.z > 0.0 && p.z <= vb_t && p.z >= vb_b && i < maxStep && d.x - d.w < dist; i += 1.0) {
        
            
            vec4
                cs = cloud_shape(p.z, profile, dens_thres);

            float
                d1 = chunk(p, flowA, flowB, cs.x),
                d2 = detial(p, flowC, flowD),
                dens = clamp(d1*(d2+1.0)+d2/5.0-0.1, 0.0, 1.0),
                marchStep = clampMap(dens, cs.z, dCut, min_sl, max_sl) * clampMap(d.x, 0.0, 10000.0, 1.0, step_mul);
                
            //marching
            p += cam_dir * marchStep;
            d.x += marchStep;
            d.w = marchStep;

            dens = clampMap(dens, cs.z, cs.y, 0.0, 1.0) * cs.w * marchStep;
            if(dens<0.01)
             continue;

            //fade into near objects
            if (d.x >= dist)
                dens *= d.z / d.w;

            col += brightness(p, profile, dens_thres, flowA, flowB, flowC) * dens * fx.z * LightDecay(d.x*0.00003, 0.0); 


            d.y = d.y * (1.0 - fx.z) + fx.z * d.x;

            fx.y += dens;
            fx.z = (exp(-fx.y / BeerFactor) - tCut) / (1.0 - tCut);

            d.z = dist - d.x;

            if(d.x>fade_e)
                break;
        }
        

        if (fx.y > 0.0) {

            // vec3 z_pos = vec3(0.0, 0.0, p.z);
            // vec3 sun_samp_pos = Game2Atm(z_pos);
            // vec3 lightDir = vec3(0.0);
            // vec3 light = LightSource(SunDirection, lightDir);
            // vec3
            //     c = SunLight(light, sun_samp_pos, SunDirection, earth);

            // vec3 sky_col = AtmosphereRadient(sun_samp_pos, vec4(cam_dir, 1.0), cam_dir, light, SunDirection, 1.0, earth)*(1.0-fx.z);

            // vec3 C = c * col * MiePhase(suncross) * cloudBrightnessMultiply + sky_col*1.5;

            // //fading
            // new_depth = d.y*fx.y + depth*(1.0-fx.y);
        
            // return vec4(C, fx.z);
        }
    }
    new_depth = depth;
    return vec4(0.0, 0.0, 0.0, 1.0);
}


void main(void) {

    vec4 ndc = vec4(gl_FragCoord.x / windowWidth * 2.0 - 1.0, gl_FragCoord.y / windowHeight * 2.0 - 1.0, gl_FragCoord.z * 2.0 - 1.0, 1.0);
    vec4 ec = InvVP * ndc;
    ec = ec / ec.w - camera;
    vec3 fin = normalize(ec.xyz);
    
    vec3 worldPosition = (InvVP*ProjectionMatrix*ModelViewMatrix*position).xyz;
    vec3 cpos = worldPosition-camera.xyz;

    float depth = length(cpos);

    vec3 direction = fin.xyz;
    
    skybot = lerp(skybot2, skybot, SunDirection.z);
    skytop = lerp(skytop2, skytop, SunDirection.z);

    // Lighting
    vec3 lightDirection = vec3(0.0);
    vec3 sunLightSource = LightSource(SunDirection, lightDirection);
    vec3 ambientLighting = Sky(vec3(0.0), Game2Atm_Alt(camera.xyz), vec3(0.0,0.0,1.0), SunDirection, earth);
    vec3 light = 0.8*SunLight(sunLightSource, Game2Atm_Alt(worldPosition.xyz), lightDirection, earth);
    
    float ambient = smoothstep(-0.2, 0.6, lightDirection.z);
    float dotSun = smoothstep(0.0,1.0,dot(-wNorm.xyz, lightDirection))*(1.0-ambient)+ambient*0.5;
    
    vec3 c = texture(sampler0, TexCoord).xyz * light * pow(dotSun, 1.0)*vec3(1.8, 1.65, 1.35);
    
    if(depth>13000.0)
        c = OutterSpace(direction, SunDirection);
    c = Sky(c, camera.xyz, direction, SunDirection, earth);
    
    float depth_c;
    vec4 midCloud = Cloud_mid(direction, camera.xyz, depth, depth, iTime*speed, 0.0, SunDirection.z, 1.0, depth_c);
    // c = c*midCloud.w + midCloud.xyz;
    
    FragColor = vec4(c, 1.0);

    // vec3 text3d = texture(
    //     test, 
    //     vec3(gl_FragCoord.x / windowWidth, 
    //     gl_FragCoord.y / windowHeight,
    //     SunDirection.z)).xyz;

    // vec3 text2d = texture(
    //     sampler0, 
    //     vec2(gl_FragCoord.x / windowWidth, 
    //     gl_FragCoord.y / windowHeight)).xyz;
        
    // FragColor = vec4(text3d, 1.0);
}
