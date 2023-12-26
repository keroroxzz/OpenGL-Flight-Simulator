#include "TextureGenerator.h"

TextureGenerator::TextureGenerator(int width, int height, int depth, GLenum internalformat, GLint warpType):
 width(width), height(height), depth(depth), internalformat(internalformat), warpType(warpType){

    // initialize computation shader
    shader = new Shader(1);
    shader->addFromFile("shaders/compute/textgen.glsl", GL_COMPUTE_SHADER);

    // initailize texture type and level
    if (depth > 1)
        textureType = GL_TEXTURE_3D;
    else if (height > 1)
        textureType = GL_TEXTURE_2D;
    else
        textureType = GL_TEXTURE_1D;

    if (textureType == GL_TEXTURE_3D)
        leveled = GL_TRUE;

    // generate texture
    glGenTextures(1, &texture);
    glBindTexture(textureType, texture);

    glTexParameteri(textureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(textureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(textureType, GL_TEXTURE_WRAP_S, warpType);
    if (textureType != GL_TEXTURE_1D)
        glTexParameteri(textureType, GL_TEXTURE_WRAP_T, warpType);
    if (textureType == GL_TEXTURE_3D)
        glTexParameteri(textureType, GL_TEXTURE_WRAP_R, warpType);

    glTexStorage3D(textureType, 1, internalformat,  width, height, depth);

    // reset texture binding
    glBindTexture(textureType, 0);
}

void TextureGenerator::reloadShaders(){
    delete shader;
    shader = new Shader(1);
    shader->addFromFile("shaders/compute/textgen.glsl", GL_COMPUTE_SHADER);
}

void TextureGenerator::generateTexture(){
    static float time = 0;
    time += 0.01;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader->use();
    shader->setUniform("time", UNI_FLOAT_1, &time);
    glBindImageTexture(0, texture, 0, leveled, 0, GL_READ_ONLY, internalformat);
    bindTexture(1);
    glDispatchCompute(width, height, depth);
    glFinish();
}

void TextureGenerator::bindTexture(GLuint unit){
    glActiveTexture(unit);
    glBindTexture(textureType, texture);
    glBindImageTexture(unit, texture, 0, leveled, 0, GL_WRITE_ONLY, internalformat);
}