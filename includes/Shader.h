#pragma once

#include "baseHeader.h"

#define UNI_INT_1 -1
#define UNI_FLOAT_1 0
#define UNI_VEC_1   1
#define UNI_VEC_2   2
#define UNI_VEC_3   3
#define UNI_VEC_4   4
#define UNI_MATRIX_2    5
#define UNI_MATRIX_3    6
#define UNI_MATRIX_4    7
#define UNI_TEXTURE    8


class Shader
{
private:
	const int maxShaders;

	int currentIndex;
	GLint program;
	GLint* shaders;
	GLboolean validation;

public:
	Shader();
	Shader(int max_num);
	~Shader();
	bool addFromFile(const char* filename, int type);
	void use();
	GLint uniformLocation(const char* uniform);
	bool setUniform(const char* uniform, int type, void* value, int count=1, GLboolean Transpose = GL_FALSE, GLint texture=0);

	GLint getProgram();

private:
	GLchar* loadShaderText(const char* fileName);
	void compileShader(const char* shadername, int index, int shader_type);
	void attachShader(int index);
	void validate();
};

