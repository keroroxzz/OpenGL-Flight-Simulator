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

#include<./shaders/common/atmosphere_settings.fs>
#include<./shaders/common/casting.fs>
#include<./shaders/common/basic.fs>
#define M_PI 3.1415926535897932384626433832795

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

vec3 LightDecay(vec2 density)
{
    // why do we need to clamp here?
    return clamp(vec3(exp(-density.x/scatteringFactor - density.y*mieDecay)), 0.0, 1.0);
}

vec4 getVerticalVecHeight(vec3 atmpos)
{
    float r = length(atmpos);
    float height = (r-groundHeight)/(atmosphereRadius-groundHeight);
    return vec4(atmpos/r, height);
}

vec2 sampleAtmosphere_txt_norm(vec3 atmpos, vec3 dir)
{
    vec3 gstart, gend;
    vec2 density = vec2(0.0);
    sphereCast(atmpos, dir, groundSphere, gstart, gend);
    if (gstart != vec3(0.0)){
        density += length(gstart-gend)*vec2(oxygenDensitySolid, vaporDensitySolid);
    }

    vec4 atmHeight = getVerticalVecHeight(atmpos);
    vec2 texcoord = vec2(atmHeight.w, acos(dot(atmHeight.xyz, dir))/M_PI);
    return density+texture(atmosphere_texture, texcoord).xy;
}
vec2 sampleAtmosphere_txt_exp(vec3 atmpos, vec3 dir)
{
    vec3 gstart, gend;
    vec2 density = vec2(0.0);
    sphereCast(atmpos, dir, groundSphere, gstart, gend);
    if (gstart != vec3(0.0)){
        density += length(gstart-gend)*vec2(oxygenDensitySolid, vaporDensitySolid);
    }

    vec4 atmHeight = getVerticalVecHeight(atmpos);
    vec2 texcoord = vec2(pow(atmHeight.w, 0.5), pow(acos(dot(atmHeight.xyz, dir))/M_PI, 2));
    return density+texture(atmosphere_texture, texcoord).xy;
}

vec2 sampleAtmosphere_txt_horiz(vec3 atmpos, vec3 dir)
{
    vec2 density = vec2(0.0);

    // calculate density beneath the ground
    vec3 gstart, gend;
    sphereCast(atmpos, dir, groundSphere, gstart, gend);
    if (gstart != vec3(0.0)){
        density = length(gstart-gend)*vec2(oxygenDensitySolid, vaporDensitySolid);
    }

    // sample the density above the ground from the texture
    // the first step is coordinate mapping
    vec4 atmHeight = getVerticalVecHeight(atmpos);
    float height_ratio = 
        clamp(1.0/((atmosphereRadius/groundHeight-1.0)*atmHeight.w+1.0), 0.0, 1.0); 
        // ensure the height_ratio is in [0,1]
    float horizontal_angle = M_PI-asin(height_ratio);
    float angle = acos(clamp(dot(atmHeight.xyz, dir), -1.0, 1.0));

    // center the texture coordinate at the horizon, so that we can sanple higher resolution 
    // near the horizon
    vec2 texcoord = vec2(atmHeight.w, 0.0);
    if (angle>horizontal_angle)
        texcoord.y = 0.5+(angle-horizontal_angle)/(M_PI-horizontal_angle)*0.5;
    else
        texcoord.y = (angle)/(horizontal_angle)*0.5;

    // non-linear mapping, to make the resolution higher near the horizon
    // this should be the inverse of the mapping in textcoordToPosRay
    texcoord.x = pow(texcoord.x, 0.5);
    texcoord.y = sign(texcoord.y-0.5)*pow(abs(texcoord.y-0.5), 0.5);
    texcoord.y = 0.707106781187*texcoord.y+0.5;

    // sample atmosphere texture
    density += texture(atmosphere_texture, texcoord).xy;

    return density;
}

/*=======================================
    Real-time Atmosphere Sampling Method
========================================*/

float Density(vec3 pos, vec4 sphere, float strength, float solidStrength, float condense)
{
    float h = length(pos-sphere.xyz)-groundHeight;
    float ep = exp(-(sphere.w-groundHeight)*condense);
    
    if (h<0.0)
        h=0.0;
    
    if (h>sphere.w-groundHeight)
        return 0.0;

    return (exp(-h*condense)-ep)/(1.0-ep)*strength;
}

vec2 IntegrateDensity(vec3 marchPos, vec3 marchStep, float atmosphereStep, vec4 sphere)
{
    float step_len = length(marchStep);
    vec2 density = vec2(0.0);
    vec2 pamount = vec2(0.0);
    for(float i=0.0; i<atmosphereStep; i++)
    {
        float oxygen = Density(
            marchPos, sphere, oxygenDensity, oxygenDensitySolid, oxygenCondensitivity);
        float vapor = Density(
            marchPos, sphere, vaporDensity, vaporDensitySolid, vaporCondensitivity);
        density += (pamount+vec2(oxygen, vapor))*step_len/2.0;
        pamount = vec2(oxygen, vapor);
        marchPos += marchStep.xyz;
    }
    
    return density;
}

vec2 sampleAtmosphere_rt(vec3 atmpos, vec3 dir)
{
    vec3 start, end;
    vec3 gstart, gend;
    sphereCast(atmpos, dir, atmosphereSphere, start, end);
    sphereCast(atmpos, dir, groundSphere, gstart, gend);

    vec2 density = vec2(0.0);
    if (gstart != vec3(0.0)){
        density += IntegrateDensity(atmpos, (gstart-start)/totalStep, totalStep, atmosphereSphere);
        density += IntegrateDensity(gend, (end-gend)/totalStep, totalStep, atmosphereSphere);
        density += length(gstart-gend)*vec2(oxygenDensitySolid, vaporDensitySolid);
    } else {
        vec3 step = (end-start)/totalStep;
        density += IntegrateDensity(start, step, totalStep, atmosphereSphere);
    }
    return density;
}

/*=======================================
    Real-time Atmosphere Sampling Method
========================================*/

#define sampleAtmosphere sampleAtmosphere_txt_horiz
// #define sampleAtmosphere sampleAtmosphere_rt
// #define sampleAtmosphere sampleAtmosphere_txt
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

    vec3 marchStep = (end - start)/atmosphereStep;
    vec3 marchPos = start;
    vec2 lastViewToPoint = vec2(0.0);
    vec2 throughShell = sampleAtmosphere(start, ray);
    
    for(float i=0.0; i<atmosphereStep; i++)
    {
        marchPos+=marchStep.xyz;
        vec2 marchPosToEnd = sampleAtmosphere(marchPos, ray);
        vec2 viewToPoint = throughShell - marchPosToEnd;
        vec2 pointToSun = sampleAtmosphere(marchPos-marchStep.xyz/2.0, lightDirection);
        vec2 viewToSun = (pointToSun + viewToPoint);

        vec2 density = (viewToPoint - lastViewToPoint);
        intensity += LightDecay(viewToSun)*(raylei*density.x + mie*density.y);
        intensity += 0.0000001/scatteringFactor*LightDecay(vec2(0.0, pointToSun.y));
        lastViewToPoint = viewToPoint;
    }
    vec3 ret = lightStrength*intensity;
    ret += background*LightDecay(throughShell) + 0.004/scatteringFactor*LightDecay(vec2(0.0, throughShell.y));
    return ret;
}

vec3 Game2Atm(vec3 pos)
{
    return pos/game2atm-atmOrigin;
}

#define M_PI 3.1415926535897932384626433832795
void textcoordToPosRay(vec2 texcoord, out vec3 pos, out vec3 ray)
{
    // non-linear mapping, to make the resolution higher near the horizon
    texcoord.x = pow(texcoord.x, 2);
    texcoord.y = sign(texcoord.y-0.5)*pow(texcoord.y-0.5, 2);
    texcoord.y = 2*texcoord.y+0.5;

    // calculate the position with height varing against the texcoord.x
    pos = vec3(
        0.0,
        0.0,
        mix(groundHeight, atmosphereRadius, texcoord.x));
    
    // calculate the ray with its angle varing against the texcoord.y
    float angle = 0.0;
    float horizontal_angle = M_PI-asin(groundHeight/pos.z);
    if (texcoord.y < 0.5)
        angle = l(0.0, horizontal_angle, texcoord.y*2.0);
    else
        angle = l(horizontal_angle, M_PI, (texcoord.y-0.5)*2.0);
    ray = vec3(sin(angle), 0.0, cos(angle));
}

void main(void) {
    vec4 ndc = vec4(gl_FragCoord.x / windowWidth, gl_FragCoord.y / windowHeight, 0.0, 0.0);

    if (ndc.x > 0.5 && ndc.y > 0.5)
    {
        ndc.xy = (ndc.xy -0.5)*2.0;
        vec3 startPos;
        vec3 ray;
        textcoordToPosRay(ndc.xy, startPos, ray);
        vec2 rt = sampleAtmosphere_rt(startPos, ray);

        // vec2 txt = sampleAtmosphere_txt_horiz(startPos, ray);
        vec2 txt = texture(atmosphere_texture, ndc.xy).xy;
        // vec2 v = abs(txt - rt)*100.0;
        vec2 v = txt;
        // vec2 v = rt;
        vec2 ramp = v*vec2(1.0,0.0);
        // FragColor = vec4(fract(ramp),0.0,0.0);
        FragColor = vec4(ramp,0.0,0.0);
        return;
    } else if (ndc.x < 0.5 && ndc.y > 0.5)
    {
        float f = 1.0;
        vec3 direction = normalize(vec3(0.5,0.5,0.0)+vec3((ndc.xy-vec2(0.25, 0.75))/0.25, f)).zxy;
        vec3 sun = sunLightStrength*vec3(smoothstep(0.9999, 1.0, dot(SunDirection,direction)));
        vec3 c = AtmosphereScattering(sun, Game2Atm(vec3(0.0,0.0,1500.0)), direction, sunLightStrength, SunDirection);
        
        FragColor = vec4(c, 1.0);
        return;
    }

    vec3 direction = normalize(wNorm);
    vec3 sun = sunLightStrength*vec3(smoothstep(0.9999, 1.0, dot(SunDirection,direction)));
    vec3 c = AtmosphereScattering(sun, Game2Atm(camera.xyz), direction, sunLightStrength, SunDirection);
    
    FragColor = vec4(c, 1.0);
}
