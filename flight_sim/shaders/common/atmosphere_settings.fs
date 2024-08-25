const float atmosphereStep = 25.0;
const float fix=0.0000001;

vec3 sunLightStrength = 685.0*vec3(1.0, 0.96, 0.949);

const float rayleighStrength = 30.0;
const float rayleighDecay = 8000.0;

const float mieStrength = 25.0;
const float mieDecay = 15000.0;

const vec3 waveLengthFactor = vec3(1750.0625, 800.3056, 300.0625);
const vec3 scatteringFactor = waveLengthFactor/rayleighDecay;

const float ozoneHeight = 6.400;
const float ozoneThickness = 0.0005;
const float ozoneDensity = 0.8;
const float ozoneChappuisAbsorptionFator = 2000.0;
const vec3 ozoneWaveLengthFactor = vec3(250, 60, 99999);
// const vec3 ozoneWaveLengthFactor = vec3(9999, 9999, 9999);
const vec3 ozoneAbsorptionFactor = ozoneChappuisAbsorptionFator/ozoneWaveLengthFactor;


const float atmosphereRadius = 6.421;
const float groundHeight = 6.371;
const vec3 atmOrigin = vec3(0.0, 0.0, -groundHeight);
const vec4 groundSphere = vec4(0.0, 0.0, 0.0, groundHeight);
const vec4 atmosphereSphere = vec4(0.0, 0.0, 0.0, atmosphereRadius);
const float game2atm = 10000000.0;

const float moonReflecticity = 0.16;
const float starStrength = 0.125;

const float oxygenDensity = 0.4;
const float oxygenDensitySolid = 0.01;
const float oxygenCondensitivity = 3.0;

const float vaporDensity = 0.00016;
const float vaporDensitySolid = 0.005;
const float vaporCondensitivity = 50.0;

const float totalStep = 1024.0;