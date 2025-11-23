#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>
#include <vector>
#include <cmath>
#include <iostream>
