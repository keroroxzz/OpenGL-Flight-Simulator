#include "Shader.h"
#include "baseHeader.h"

#define _CRT_SECURE_NO_WARNINGS
#define FREEGLUT_STATIC
#define MAX_INFO_LOG_SIZE 2048

// Constructor
Shader::Shader(int max_num)
    : maxShaders(max_num), currentIndex(0), validation(GL_FALSE) {
  shaders = new GLint[maxShaders];

  // init shader program
  program = glCreateProgram();

  LOGI("Shader Manager created!");
}

Shader::~Shader() {
  glDeleteProgram(program);
  for (int i = 0; i < currentIndex; i++)
    glDeleteShader(shaders[i]);

  LOGD("Shader Manager destroyed!");
  delete[] shaders;
}

bool Shader::addFromFile(const char* filename, int type) {
  if (currentIndex >= maxShaders)
    return 0;

  compileShader(Path(filename), currentIndex, type);
  attachShader(currentIndex);
  validate();

  currentIndex++;

  LOGI("Add shader: %s", filename);
  return 1;
}

void Shader::use() {
  glUseProgram(program);
  // LOGD("Use shader: %d", program);
}

std::string Shader::loadShaderText(const char* fileName, std::vector<Path> include_chain) {

  // prevent include loop
  for (Path & path: include_chain) {
    if (path == Path(fileName)) {
      LOGE("Include loop detected: %s", path.operator const char *());
      return "";
    }
  }

  // load shader text
  GLchar* text = NULL;
  GLint length = 0;

  FILE* fp = fopen(fileName, "r");
  if (fp != NULL) {
    // calculate the length of file
    while (fgetc(fp) != EOF)
      length++;
    rewind(fp);

    text = (GLchar*)malloc(length + 1);
    if (text != NULL)
      fread(text, 1, length, fp);
    text[length] = '\0';

    // remove comments, and newlines for included shaders
    if (text != NULL && include_chain.size() > 0) {
      bool is_comment = false;
      for (int i=0;i<length;i++) {
        if (i<length-1 && text[i] == '/' && text[i+1] == '/') {
          is_comment = true;
        }
        if (text[i] == '\n') {
          text[i] = ' ';
          is_comment = false;
        }
        else if (is_comment) {
          text[i] = ' ';
        }
      }
    }

    fclose(fp);
  } else {
    LOGE("Unable to load \"%s\"", fileName);
    return NULL;
  }

  if (text == NULL)
    return "";

  std::string str(text);
  free(text);

  // include other shaders
  int beg = str.length()-1;
  while (true) {
    beg = str.rfind("#include<", beg);
    if (beg == std::string::npos)
      break;
    int end = str.find(">", beg);
    if (end == std::string::npos)
      break;
    Path include_path = Path(str.substr(beg + 9, end - beg - 9).c_str());
    include_chain.push_back(Path(fileName));
    LOGD("Include shader: %s", include_path.operator const char *());
    std::string included_shader = loadShaderText(include_path, include_chain);
    str.replace(beg, end - beg + 1, included_shader);
  }

  return str;
}

void Shader::compileShader(const char* shadername, int index, int shader_type) {
  std::string fsString;
  const GLchar* fsStringPtr[1];
  GLint success;

  LOGD("Compile Shader: %s", shadername);
  fsString = loadShaderText(shadername);


  shaders[index] = glCreateShader(shader_type);
  fsStringPtr[0] = fsString.c_str();
  glShaderSource(shaders[index], 1, fsStringPtr, NULL);

  // Compile shaders and check for any errors
  glCompileShader(shaders[index]);
  glGetShaderiv(shaders[index], GL_COMPILE_STATUS, &success);

  if (!success) {
    GLchar infoLog[MAX_INFO_LOG_SIZE];
    glGetShaderInfoLog(shaders[index], MAX_INFO_LOG_SIZE, NULL, infoLog);
    LOGE("Error in fragment shader #%d compilation!", index);
    LOGE("Info log: %s", infoLog);
    std::cout << infoLog << std::endl;
    return;
  }
}

void Shader::attachShader(int index) {
  GLint success;

  LOGD("Attach Shader: %d", index);
  glAttachShader(program, shaders[index]);

  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &success);

  if (!success) {
    GLchar infoLog[MAX_INFO_LOG_SIZE];
    glGetProgramInfoLog(program, MAX_INFO_LOG_SIZE, NULL, infoLog);
    LOGE("Error in program #%d linkage!", index);
    LOGE("Info log: %s", infoLog);
    std::cout << infoLog << std::endl;
    return;
  }

  // Program object has changed, so we should revalidate
  validation = GL_FALSE;
}

void Shader::validate() {
  if (!validation) {
    GLint success;

    glValidateProgram(program);
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
    if (!success) {
      GLchar infoLog[MAX_INFO_LOG_SIZE];
      glGetProgramInfoLog(program, MAX_INFO_LOG_SIZE, NULL, infoLog);
      LOGE("Error in program validation!");
      LOGE("Info log: %s", infoLog);
      return;
    }

    validation = GL_TRUE;
  }
}

GLint Shader::uniformLocation(const char* uniform) {
  return glGetUniformLocation(program, uniform);
}

bool Shader::setUniform(const char* uniform,
                        int type,
                        void* value,
                        int count,
                        GLboolean Transpose,
                        GLint texture) {
  GLint i = glGetUniformLocation(program, uniform);

  if (i == -1) {
    // std::cout << "Uniform : " << uniform << " is not found!\n";
    return false;
  }

  switch (type) {
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

GLint Shader::getProgram() {
  return program;
}