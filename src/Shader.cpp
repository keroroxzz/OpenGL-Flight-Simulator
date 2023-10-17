#include "baseHeader.h"
#include "Shader.h"

#define _CRT_SECURE_NO_WARNINGS
#define FREEGLUT_STATIC
#define MAX_INFO_LOG_SIZE 2048

//Constructor
Shader::Shader(int max_num) : maxShaders(max_num), currentIndex(0), validation(GL_FALSE)
{
    shaders = new GLint[maxShaders];

    //init shader program
    program = glCreateProgram();
}

Shader::~Shader()
{
    glDeleteProgram(program);
    for (int i = 0; i < currentIndex; i++)
        glDeleteShader(shaders[i]);

    delete[] shaders;
}


bool Shader::addFromFile(const char* filename, int type)
{
    if (currentIndex >= maxShaders)
        return 0;

    compileShader(filename, currentIndex, type);
    attachShader(currentIndex);
    validate();

    currentIndex++;

    return 1;
}

void Shader::use()
{
    glUseProgram(program);
}

GLchar* Shader::loadShaderText(const char* fileName)
{
    GLchar* text = NULL;
    GLint length = 0;

    FILE* fp = fopen(fileName, "r");
    if (fp != NULL)
    {
        //calculate the length of file
        while (fgetc(fp) != EOF)
            length++;
        rewind(fp);

        text = (GLchar*)malloc(length + 1);
        if (text != NULL)
            fread(text, 1, length, fp);
        text[length] = '\0';

        fclose(fp);
    }
    else
    {
        fprintf(stderr, "Unable to load \"%s\"\n", fileName);
        return NULL;
    }

    return text;
}

void Shader::compileShader(const char* shadername, int index, int shader_type)
{
    GLchar* fsString;
    const GLchar* fsStringPtr[1];
    GLint success;

    printf("Compile Shader: %s\n", shadername);
    fsString = loadShaderText(shadername);

    if (fsString == NULL) return;

    shaders[index] = glCreateShader(shader_type);
    fsStringPtr[0] = fsString;
    glShaderSource(shaders[index], 1, fsStringPtr, NULL);
    free(fsString);

    // Compile shaders and check for any errors
    glCompileShader(shaders[index]);
    glGetShaderiv(shaders[index], GL_COMPILE_STATUS, &success);

    if (!success)
    {
        GLchar infoLog[MAX_INFO_LOG_SIZE];
        glGetShaderInfoLog(shaders[index], MAX_INFO_LOG_SIZE, NULL, infoLog);
        fprintf(stderr, "Error in fragment shader #%d compilation!\n", index);
        fprintf(stderr, "Info log: %s\n", infoLog);
        std::cout << infoLog << std::endl;
        return;
    }

}

void Shader::attachShader(int index)
{
    GLint success;

    printf("Attach Shader: %d\n", index);
    glAttachShader(program, shaders[index]);

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        GLchar infoLog[MAX_INFO_LOG_SIZE];
        glGetProgramInfoLog(program, MAX_INFO_LOG_SIZE, NULL, infoLog);
        fprintf(stderr, "Error in program #%d linkage!\n", index);
        fprintf(stderr, "Info log: %s\n", infoLog);
        std::cout << infoLog << std::endl;
        return;
    }

    // Program object has changed, so we should revalidate
    validation = GL_FALSE;
}

void Shader::validate()
{
    if (!validation)
    {
        GLint success;

        glValidateProgram(program);
        glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
        if (!success)
        {
            GLchar infoLog[MAX_INFO_LOG_SIZE];
            glGetProgramInfoLog(program, MAX_INFO_LOG_SIZE, NULL, infoLog);
            fprintf(stderr, "Error in program validation!\n");
            fprintf(stderr, "Info log: %s\n", infoLog);
            return;
        }

        validation = GL_TRUE;
    }
}

GLint Shader::uniformLocation(const char* uniform)
{
    return glGetUniformLocation(program, uniform);
}

bool Shader::setUniform(const char* uniform, int type, void* value, int count, GLboolean Transpose, GLint texture)
{
    GLint i = glGetUniformLocation(program, uniform);

    if (i == -1)
    {
        // std::cout << "Uniform : " << uniform << " is not found!\n";
        return false;
    }

    switch (type)
    {
    case UNI_INT_1:
        glUniform1i(i, *(GLint*)value);
        break;
    case UNI_FLOAT_1:
        glUniform1f(i, *((GLfloat*)value));
        break;
    case UNI_VEC_1:
        glUniform1fv(i, count, (GLfloat*)value);
        break;
    case UNI_VEC_2:
        glUniform2fv(i, count, (GLfloat*)value);
        break;
    case UNI_VEC_3:
        glUniform3fv(i, count, (GLfloat*)value);
        break;
    case UNI_VEC_4:
        glUniform4fv(i, count, (GLfloat*)value);
        break;
    case UNI_MATRIX_2:
        glUniformMatrix2fv(i, count, Transpose, (GLfloat*)value);
        break;
    case UNI_MATRIX_3:
        glUniformMatrix3fv(i, count, Transpose, (GLfloat*)value);
        break;
    case UNI_MATRIX_4:
        glUniformMatrix4fv(i, count, Transpose, (GLfloat*)value);
        break;
    case UNI_TEXTURE:
        glUniform1i(i, texture);
        break;
    }

    return true;
}

GLint Shader::getProgram()
{
    return program;
}