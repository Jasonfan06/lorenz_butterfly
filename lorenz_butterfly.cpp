/*
 * Lorenz Butterfly - Clean Rewrite
 * Real-time visualization with smooth colors and simple rendering
 */

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
#include <cstdio>
#include <memory>
#include <ctime>

// ============================================================================
// CONFIGURATION
// ============================================================================

const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const int N_TRAJECTORIES = 40;
const int MAX_TRAIL_LENGTH = 1000;

// Lorenz parameters
const float SIGMA = 10.0f;
const float RHO = 28.0f;
const float BETA = 8.0f / 3.0f;

// Simulation
const float DT = 0.005f;
const float TOTAL_TIME = 108.0f;

// ============================================================================
// LORENZ SYSTEM
// ============================================================================

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

// ============================================================================
// COLOR SYSTEM
// ============================================================================

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

// ============================================================================
// TRAJECTORY
// ============================================================================

class Trajectory {
public:
    std::vector<Vec3> full_path;  // Complete computed path
    std::vector<Vec3> visible_trail;  // Current visible trail
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

// ============================================================================
// GLOBAL STATE
// ============================================================================

std::vector<Trajectory> trajectories;
float current_time = 0.0f;
bool paused = false;
int total_steps = 0;
float camera_distance = 120.0f;  // Current zoom
float target_camera_distance = 120.0f;  // Target zoom (interpolated to)
const float ZOOM_MIN = 60.0f;
const float ZOOM_MAX = 250.0f;
const float ZOOM_SPEED = 0.05f;  // Slower interpolation for smoother, longer transitions

// Video recording
bool recording = false;
FILE* ffmpeg_pipe = nullptr;
std::vector<unsigned char> frame_buffer;
int frame_count = 0;
int record_width = 0;
int record_height = 0;
GLFWwindow* global_window = nullptr;  // Store window handle for recording

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

void start_recording();
void stop_recording();
void capture_frame();

// ============================================================================
// RENDERING
// ============================================================================

void setup_camera() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    float aspect = (float)WINDOW_WIDTH / WINDOW_HEIGHT;
    float fov = 40.0f * 3.14159f / 180.0f;
    float near_plane = 1.0f;
    float far_plane = 500.0f;
    float top = near_plane * tanf(fov / 2.0f);
    
    glFrustum(-top * aspect, top * aspect, -top, top, near_plane, far_plane);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Rotating camera with adjustable zoom
    float angle = current_time / 2.0f;
    float cam_x = camera_distance * cosf(angle);
    float cam_y = camera_distance * sinf(angle);
    float cam_z = 25.0f;
    
    gluLookAt(cam_x, cam_y, cam_z,    // Eye
              0, 0, 25,                 // Look at center
              0, 0, 1);                 // Up
}

void draw_trail(const Trajectory& traj) {
    if (traj.visible_trail.size() < 2) return;
    
    glLineWidth(4.0f);
    glBegin(GL_LINE_STRIP);
    
    int n = traj.visible_trail.size();
    for (int i = 0; i < n; i++) {
        // Fade from transparent to opaque
        float alpha = (float)i / n;
        alpha = alpha * alpha;  // Smooth fade
        
        glColor4f(traj.color.r, traj.color.g, traj.color.b, alpha * 0.9f);
        
        const Vec3& p = traj.visible_trail[i];
        glVertex3f(p.x, p.y, p.z);
    }
    
    glEnd();
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    setup_camera();
    
    // Enable nice line rendering
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    
    // Disable depth writes so lines don't occlude each other
    // This creates smooth overlapping trails
    glDepthMask(GL_FALSE);
    
    // Use additive blending for a smooth "neon glow" effect
    // Lines add together instead of covering each other
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    
    // Draw all trajectories
    for (const auto& traj : trajectories) {
        draw_trail(traj);
    }
    
    // Restore depth mask
    glDepthMask(GL_TRUE);
    
    // Capture frame if recording
    if (recording) {
        capture_frame();
    }
}

// ============================================================================
// SIMULATION
// ============================================================================

void init_simulation() {
    trajectories.clear();
    trajectories.resize(N_TRAJECTORIES);
    
    total_steps = (int)(TOTAL_TIME / DT);
    
    std::cout << "Computing trajectories..." << std::endl;
    
    for (int i = 0; i < N_TRAJECTORIES; i++) {
        // Initial condition: vary y coordinate
        Vec3 initial_pos(0.0f, (i + 1.0f) / N_TRAJECTORIES, 0.0f);
        
        trajectories[i].compute(initial_pos, total_steps);
        trajectories[i].color = get_color((float)i / N_TRAJECTORIES);
        
        std::cout << "  Trajectory " << (i + 1) << "/" << N_TRAJECTORIES << std::endl;
    }
    
    std::cout << "Ready!" << std::endl;
}

void update() {
    // Smooth zoom interpolation (always active, even when paused)
    camera_distance += (target_camera_distance - camera_distance) * ZOOM_SPEED;
    
    if (paused) return;
    
    bool all_done = true;
    for (auto& traj : trajectories) {
        if (traj.current_step < traj.full_path.size()) {
            traj.update();
            all_done = false;
        }
    }
    
    // Loop when done
    if (all_done) {
        for (auto& traj : trajectories) {
            traj.reset();
        }
        current_time = 0.0f;
    } else {
        current_time += DT;
    }
}

// ============================================================================
// VIDEO RECORDING
// ============================================================================

void start_recording() {
    if (recording || !global_window) return;
    
    // Get actual framebuffer size (important for Retina/HiDPI displays)
    glfwGetFramebufferSize(global_window, &record_width, &record_height);
    
    // Generate filename with timestamp
    time_t now = time(nullptr);
    char filename[256];
    strftime(filename, sizeof(filename), "lorenz_%Y%m%d_%H%M%S.mp4", localtime(&now));
    
    // Construct FFmpeg command with actual framebuffer dimensions
    char command[512];
    snprintf(command, sizeof(command),
        "ffmpeg -y -f rawvideo -pixel_format rgb24 -video_size %dx%d -framerate 60 "
        "-i - -c:v libx264 -preset fast -crf 18 -pix_fmt yuv420p %s",
        record_width, record_height, filename);
    
    ffmpeg_pipe = popen(command, "w");
    
    if (ffmpeg_pipe) {
        recording = true;
        frame_buffer.resize(record_width * record_height * 3);
        frame_count = 0;
        std::cout << "\n🔴 Recording started: " << filename 
                  << " (" << record_width << "x" << record_height << ")" << std::endl;
    } else {
        std::cerr << "Failed to start FFmpeg. Make sure it's installed." << std::endl;
    }
}

void stop_recording() {
    if (!recording) return;
    
    recording = false;
    
    if (ffmpeg_pipe) {
        pclose(ffmpeg_pipe);
        ffmpeg_pipe = nullptr;
    }
    
    std::cout << "⏹️  Recording stopped (" << frame_count << " frames)" << std::endl;
    frame_buffer.clear();
    frame_count = 0;
}

void capture_frame() {
    if (!recording || !ffmpeg_pipe) return;
    
    // Read pixels from actual framebuffer (matches what you see on screen)
    glReadPixels(0, 0, record_width, record_height, GL_RGB, GL_UNSIGNED_BYTE, frame_buffer.data());
    
    // Flip image vertically (OpenGL has origin at bottom-left)
    std::vector<unsigned char> flipped(frame_buffer.size());
    int row_size = record_width * 3;
    for (int y = 0; y < record_height; y++) {
        memcpy(&flipped[y * row_size], 
               &frame_buffer[(record_height - 1 - y) * row_size], 
               row_size);
    }
    
    // Write to FFmpeg pipe
    fwrite(flipped.data(), 1, flipped.size(), ffmpeg_pipe);
    frame_count++;
}

// ============================================================================
// INPUT
// ============================================================================

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    
    switch (key) {
        case GLFW_KEY_ESCAPE:
        case GLFW_KEY_Q:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_SPACE:
            if (action == GLFW_PRESS) paused = !paused;
            break;
        case GLFW_KEY_R:
            if (action == GLFW_PRESS) {
                for (auto& traj : trajectories) {
                    traj.reset();
                }
                current_time = 0.0f;
            }
            break;
        case GLFW_KEY_EQUAL:  // + key (zoom in)
        case GLFW_KEY_KP_ADD:
            target_camera_distance -= 8.0f;
            if (target_camera_distance < ZOOM_MIN) target_camera_distance = ZOOM_MIN;
            break;
        case GLFW_KEY_MINUS:  // - key (zoom out)
        case GLFW_KEY_KP_SUBTRACT:
            target_camera_distance += 8.0f;
            if (target_camera_distance > ZOOM_MAX) target_camera_distance = ZOOM_MAX;
            break;
        case GLFW_KEY_0:  // Reset zoom
            target_camera_distance = 120.0f;
            break;
        case GLFW_KEY_V:  // Toggle recording
            if (action == GLFW_PRESS) {
                if (recording) {
                    stop_recording();
                } else {
                    start_recording();
                }
            }
            break;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // Smooth zoom with mouse scroll - smaller steps for more gradual transitions
    target_camera_distance -= (float)yoffset * 10.0f;
    
    // Clamp to min/max
    if (target_camera_distance < ZOOM_MIN) target_camera_distance = ZOOM_MIN;
    if (target_camera_distance > ZOOM_MAX) target_camera_distance = ZOOM_MAX;
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Create window
    glfwWindowHint(GLFW_SAMPLES, 4);  // 4x antialiasing
    GLFWwindow* window = glfwCreateWindow(
        WINDOW_WIDTH, WINDOW_HEIGHT,
        "Lorenz Butterfly",
        nullptr, nullptr
    );
    
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    global_window = window;  // Store for recording
    
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSwapInterval(1);  // VSync - match monitor refresh rate
    
    // OpenGL setup
    glClearColor(0.04f, 0.04f, 0.04f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    
    // Initialize simulation
    init_simulation();
    
    std::cout << "\n=== Controls ===" << std::endl;
    std::cout << "SPACE       - Pause/Resume" << std::endl;
    std::cout << "R           - Restart" << std::endl;
    std::cout << "+/=         - Zoom In" << std::endl;
    std::cout << "-           - Zoom Out" << std::endl;
    std::cout << "0           - Reset Zoom" << std::endl;
    std::cout << "Mouse Wheel - Zoom In/Out" << std::endl;
    std::cout << "V           - Start/Stop Recording" << std::endl;
    std::cout << "ESC/Q       - Quit" << std::endl;
    std::cout << "\nRunning at monitor refresh rate...\n" << std::endl;
    
    // Main loop - VSync matches monitor refresh rate
    while (!glfwWindowShouldClose(window)) {
        update();
        render();
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    if (recording) {
        stop_recording();
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
