/*
    GTA SA CloudWorks by Brian Tu 
    Version : GLSL Alpha (3.6.7)
    Date : 2021/1/5
    Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License
    Please cite the author if you use this code in your work.
    
    Note : haven't optimized the code yet
*/
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aNorm;
layout (location = 8) in vec2 aTexCoord;

uniform mat3 InvNVP;
uniform mat4 ProjectionMatrix;
uniform mat4 ModelViewMatrix;

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
   position = vec4(aPos, 1.0);
   
   gl_Position = ProjectionMatrix*ModelViewMatrix*position;
}
