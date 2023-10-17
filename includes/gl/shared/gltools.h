// GLTools.h
// OpenGL SuperBible
// Copyright 1998 - 2003 Richard S. Wright Jr.
// Code by Richard S. Wright Jr.
// All Macros prefixed with GLT_, all functions prefixed with glt... This
// should avoid most namespace problems
// Some of these functions allocate memory. Use CRT functions to free
// Report bugs to rwright@starstonesoftware.com

#ifndef __GLTOOLS__LIBRARY
#define __GLTOOLS__LIBRARY

#define FREEGLUT_STATIC
#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/glext.h>

typedef GLvoid (*CallBack)(...);

// Universal includes
#include <math.h>


// There is a static block allocated for loading shaders to prevent heap fragmentation
#define MAX_SHADER_LENGTH   8192


///////////////////////////////////////////////////////////////////////////////
//         THE LIBRARY....
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
// Load a .TGA file
GLbyte *gltLoadTGA(const char *szFileName, GLint *iWidth, GLint *iHeight, GLint *iComponents, GLenum *eFormat);

// Get the OpenGL version, returns fals on error
bool gltGetOpenGLVersion(int &nMajor, int &nMinor);

// Check to see if an exension is supported
int gltIsExtSupported(const char *szExtension);

// Get the function pointer for an extension
void *gltGetExtensionPointer(const char *szFunctionName);

///////////////////////////////////////////////////////////////////////////////
// Win32 Only
#ifdef WIN32
int gltIsWGLExtSupported(HDC hDC, const char *szExtension);
#endif


#endif