#define _CRT_SECURE_NO_WARNINGS
#define FREEGLUT_STATIC

#include <pthread.h>
#include "baseHeader.h"
#include "Shader.h"
#include "ObjModel.h"
#include "Plane.h"
#include <chrono>

#include "utils/logger/includes/logger.h"
#include "utils/path_mgr/includes/path_mgr.h"

class TextureGenerator{

    GLenum textureType;
    GLenum internalformat;
    GLint leveled;
    GLint warpType;
    GLuint txtname;
    GLuint renderBuffer;
    GLuint texture;
    Shader *shader;

    unsigned int VBO[2];
    const char *shader_path;
    
    // unsigned int texname;
    int width, height, depth;
    
public:
    TextureGenerator(
        const char shader_path[],
        int width, 
        int height,
        int depth, 
        GLenum internalformat, 
        GLint warpType);

    void initFramebuffer();
    void generateTexture();
    void bindTexture(GLuint unit);
    void reloadShaders();
};