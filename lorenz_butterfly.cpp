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

struct Color {
    float r, g, b;
    Color(float r = 1, float g = 1, float b = 1) : r(r), g(g), b(b) {}
};

Color get_color(float t) {
    // Smooth saturated rainbow - no white/pastel colors
    t = fmodf(t, 1.0f);
    
    // Saturated color wheel with smooth transitions
    float h = t * 6.0f;  // 0 to 6
    int segment = (int)floorf(h);
    float frac = h - segment;
    
    // Smooth the fraction using smoothstep
    frac = frac * frac * (3.0f - 2.0f * frac);
    
    float r, g, b;
    
    switch(segment % 6) {
        case 0:  // Red to Yellow
            r = 1.0f;
            g = frac;
            b = 0.0f;
            break;
        case 1:  // Yellow to Green
            r = 1.0f - frac;
            g = 1.0f;
            b = 0.0f;
            break;
        case 2:  // Green to Cyan
            r = 0.0f;
            g = 1.0f;
            b = frac;
            break;
        case 3:  // Cyan to Blue
            r = 0.0f;
            g = 1.0f - frac;
            b = 1.0f;
            break;
        case 4:  // Blue to Magenta
            r = frac;
            g = 0.0f;
            b = 1.0f;
            break;
        default:  // Magenta to Red
            r = 1.0f;
            g = 0.0f;
            b = 1.0f - frac;
            break;
    }
    
    return Color(r, g, b);
}

class Trajectory {
public:
    std::vector<Vec3> full_path;  // computed path
    std::vector<Vec3> visible_trail;  // current visible trail
    Color color;
    int current_step;
    
    Trajectory() : current_step(0) {}
    
    void compute(Vec3 initial_pos, int total_steps) {
        full_path.clear();
        full_path.reserve(total_steps);
        
        Vec3 pos = initial_pos;
        for (int i = 0; i < total_steps; i++) {
            full_path.push_back(pos);
            pos = rk4_step(pos, DT);
        }
    }
    
    void update() {
        if (current_step < full_path.size()) {
            visible_trail.push_back(full_path[current_step]);
            if (visible_trail.size() > MAX_TRAIL_LENGTH) {
                visible_trail.erase(visible_trail.begin());
            }
            current_step++;
        }
    }
    
    void reset() {
        current_step = 0;
        visible_trail.clear();
    }
};
