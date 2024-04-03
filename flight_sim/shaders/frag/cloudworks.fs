#version 330 core
in vec4 position;
in vec3 wNorm;
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

float noise_p(vec3 p) {

    vec3 text3d = texture(
        test, p).xyz;

    return text3d.x;
}

float map(float x, float a, float b, float c, float d) {
    return (x - b) / (a - b) * (c - d) + d;
}

float atmosphereStep = 10.0;
float fix=0.0000001;

vec3 sunLightStrength = 685.0*vec3(1.0, 0.96, 0.949);

vec2 atmDensity = vec2(0.75, 0.001);
float rayleighStrength = 16.0;
const float rayleighDecay = 2000.0;

const vec3 waveLengthFactor = vec3(1785.0625, 850.3056, 410.0625);
const vec3 scatteringFactor = waveLengthFactor/rayleighDecay;

float mieStrength = 30.0;
float mieDecay = 5000.0;

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

vec3 sphereCast(vec3 origin, vec3 ray, vec4 sphere)
{
    vec3 p = origin - sphere.xyz;
    
    float 
        r = length(p),
        d = length(cross(p, ray));    //nearest distance from view-ray to the center
        
    if (d > sphere.w+fix) return vec3(0.0);    //early exit
        
    float
        sr = sqrt(sphere.w*sphere.w - d*d),    //radius of slicing circle
        dr = -dot(p, ray);    //distance from camera to the nearest point\
    
    if(r>sphere.w && dr>0){
        return origin + ray * (dr-sr);
    }else if(r<=sphere.w){
        return origin + ray * (dr+sr);
    }else{
        return vec3(0.0);
    }
}

void rayDistance(vec3 posCam, vec3 ray, out vec3 posAtm, out vec3 posGround, out vec3 posGroundBack, out float lenGnd, out float lenAtm){
    
    posAtm = sphereCast(posCam, ray, atmosphereSphere);
    posGround = sphereCast(posCam, ray, groundSphere);
    posGroundBack = sphereCast(posAtm, -ray, groundSphere);

    if (posGround!=vec3(0.0)){
        lenGnd = length(posGround-posCam);
        lenAtm = length(posAtm-posGroundBack);
    }
    else{
        lenGnd = 0.0;
        lenAtm = length(posAtm-posGroundBack);
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

vec4 getVerticalVecHeight(vec3 atmpos)
{
    vec3 relative = atmpos;
    float height = (length(relative)-groundHeight)/(atmosphereRadius-groundHeight);
    return vec4(relative/length(relative), height);
}

float clamp(float v, float min_, float max_)
{
    return min(max(v, min_), max_);
}

vec3 clamp(vec3 v, float min, float max)
{
    return vec3(clamp(v.x, min, max), clamp(v.y, min, max), clamp(v.z, min, max));
}

vec2 sampleAtmosphere(vec3 atmpos, vec3 dir, float dist)
{
    vec4 atmHeight = getVerticalVecHeight(atmpos);
    vec3 texcoord = clamp(vec3(atmHeight.w, dot(atmHeight.xyz, dir)*0.5+0.5, dist), 0.01, 0.99);
    return atmDensity*texture(atmText, texcoord).xy;
}

vec3 AtmosphereScattering(vec3 background, vec3 camPos, vec3 ray, vec3 lightStrength, vec3 lightDirection)
{
    vec3 intensity=vec3(0.0);
    
    float ang_cos = dot(ray, lightDirection);
    float mie = MieScattering(ang_cos);
    vec3 raylei = rayleighScattering(ang_cos);
    
    vec3 marchPos = camPos;
    vec3 atmPos, gndPos, gndPosBack;
    float lenGnd, lenAtm;
    rayDistance(marchPos, ray, atmPos, gndPos, gndPosBack, lenGnd, lenAtm);

    // return texture(test, gndPos*5.0).xyz*5.0;

    vec3 marchStep = ray*(lenAtm + lenGnd)/atmosphereStep;

    vec3 begPose = marchPos;

    vec2 dv = vec2(0.0);
    vec2 lastViewToPoint = vec2(0.0);
    for(float i=0.0; i<atmosphereStep; i++)
    {
        float dist = i/atmosphereStep;

        if (lenGnd>0.0 && i == int(atmosphereStep*lenGnd/(lenAtm + lenGnd))){
            marchPos=gndPosBack+ray;
        }

        vec2 viewToPoint = sampleAtmosphere(begPose, ray, dist);
        vec2 pointToSun = sampleAtmosphere(marchPos, lightDirection, 1.0);
        vec2 viewToSun = (pointToSun + viewToPoint);

        vec2 density = viewToPoint - lastViewToPoint;
        if (lenGnd>0.0 && i == int(atmosphereStep*lenGnd/(lenAtm + lenGnd))){
            density = vec2(0.0);
        }

        intensity += LightDecay(viewToSun.x, viewToSun.y)*(raylei*density.x + mie*density.y);
        lastViewToPoint = viewToPoint;

        marchPos+=marchStep.xyz;
    }
    // return 5.0*vec3(lastViewToPoint.x, lastViewToPoint.y*10.0,0.0);
    return lightStrength*intensity + background*LightDecay(lastViewToPoint.x, lastViewToPoint.y);
}

vec3 Game2Atm(vec3 pos)
{
    return pos/game2atm-atmOrigin;
}

vec3 WorldToSpace(vec3 ray, vec3 SunDirection)
{
    vec3 
        axis_x = normalize(vec3(0.85, -2.0, 0.45)),   //The rotation axis of the Earth
        axis_z = normalize(cross(axis_x, SunDirection)),
        axis_y = normalize(cross(axis_z, axis_x));
        
    return normalize(vec3(dot(axis_x, ray), dot(axis_y, ray), dot(axis_z, ray)));
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
        res += StarSky(starStrength, 300.0, 6.0, WorldToSpace(ray, SunDirection));
    }
    
    return res;
}

void main(void) {

    vec4 ndc = vec4(gl_FragCoord.x / windowWidth * 2.0 - 1.0, gl_FragCoord.y / windowHeight * 2.0 - 1.0, gl_FragCoord.z * 2.0 - 1.0, 1.0);
    vec4 ec = InvVP * ndc;
    ec = ec / ec.w - camera;
    vec3 fin = normalize(ec.xyz);

    vec3 direction = fin.xyz;

    vec3 stars = OutterSpace(direction, SunDirection);
    vec3 c = AtmosphereScattering(stars, Game2Atm(camera.xyz), direction, sunLightStrength, SunDirection);
    
    FragColor = vec4(c*0.2, 1.0);
    // FragColor = vec4(c/(3.0*smoothstep(-0.2,0.15,SunDirection.z)+0.5), 1.0);
}
