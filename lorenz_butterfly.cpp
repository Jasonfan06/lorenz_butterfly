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

struct Vec3 {
    float x, y, z;
    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};

Vec3 lorenz_derivative(const Vec3& p) {
    return Vec3(
        SIGMA * (p.y - p.x),
        RHO * p.x - p.y - p.x * p.z,
        p.x * p.y - BETA * p.z
    );
}

Vec3 rk4_step(const Vec3& p, float dt) {
    Vec3 k1 = lorenz_derivative(p);
    Vec3 k2 = lorenz_derivative(Vec3(p.x + k1.x*dt/2, p.y + k1.y*dt/2, p.z + k1.z*dt/2));
    Vec3 k3 = lorenz_derivative(Vec3(p.x + k2.x*dt/2, p.y + k2.y*dt/2, p.z + k2.z*dt/2));
    Vec3 k4 = lorenz_derivative(Vec3(p.x + k3.x*dt, p.y + k3.y*dt, p.z + k3.z*dt));
    
    return Vec3(
        p.x + (k1.x + 2*k2.x + 2*k3.x + k4.x) * dt / 6.0f,
        p.y + (k1.y + 2*k2.y + 2*k3.y + k4.y) * dt / 6.0f,
        p.z + (k1.z + 2*k2.z + 2*k3.z + k4.z) * dt / 6.0f
    );
}
