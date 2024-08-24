
float atmosphereStep = 16.0;
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
// const float groundHeight = 4.0;
const float atmosphereRadius = 6.421;
const float groundHeight = 6.371;
const vec3 atmOrigin = vec3(0.0, 0.0, -groundHeight);
const vec4 groundSphere = vec4(0.0, 0.0, 0.0, groundHeight);
const vec4 atmosphereSphere = vec4(0.0, 0.0, 0.0, atmosphereRadius);
const float game2atm = 10000000.0;

float moonReflecticity = 0.16;
float starStrength = 0.125;

float oxygenDensity = 0.3;
float oxygenDensitySolid = 0.01;
float oxygenCondensitivity = 1.0;

float vaporDensity = 0.0001;
float vaporDensitySolid = 0.001;
float vaporCondensitivity = 100.0;

float totalStep = 50.0;