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

const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const int N_TRAJECTORIES = 16;
const int MAX_TRAIL_LENGTH = 1000;

// Lorenz parameters
const float SIGMA = 10.0f;
const float RHO = 28.0f;
const float BETA = 8.0f / 3.0f;

// Simulation
const float DT = 0.005f;
const float TOTAL_TIME = 108.0f;
