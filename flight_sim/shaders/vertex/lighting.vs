#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aNorm;
layout (location = 8) in vec2 aTexCoord;

uniform mat3 InvNVP;
uniform mat4 ProjectionMatrix;
uniform mat4 ModelViewMatrix;
uniform mat4 ShadowMVP;
uniform sampler2D sampler1;

out vec3 wNorm;
out vec3 vNorm;
out vec4 position;
out vec2 TexCoord;


void main(void)  
{     
   mat3 NormalMatrix = (mat3(ModelViewMatrix));

   TexCoord = aTexCoord;
   wNorm = normalize(InvNVP * NormalMatrix * aNorm);
   vNorm = normalize(NormalMatrix * aNorm);
   
   gl_Position = ProjectionMatrix*ModelViewMatrix*vec4(aPos, 1.0);
}