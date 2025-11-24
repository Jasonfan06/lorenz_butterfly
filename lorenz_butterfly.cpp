#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/glew.h>
#endif#include <GLFW/glfw3.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <cstdio>
#include <memory>
#include <ctime>const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const int N_TRAJECTORIES = 40;
const int MAX_TRAIL_LENGTH = 1000;const float SIGMA = 10.0f;
const float RHO = 28.0f;
const float BETA = 8.0f / 3.0f;const float DT = 0.005f;
const float TOTAL_TIME = 108.0f;struct Vec3 {
    float x, y, z;
    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};Vec3 lorenz_derivative(const Vec3& p) {
    return Vec3(
        SIGMA * (p.y - p.x),
        RHO * p.x - p.y - p.x * p.z,
        p.x * p.y - BETA * p.z
    );
}Vec3 rk4_step(const Vec3& p, float dt) {
    Vec3 k1 = lorenz_derivative(p);
    Vec3 k2 = lorenz_derivative(Vec3(p.x + k1.x*dt/2, p.y + k1.y*dt/2, p.z + k1.z*dt/2));
    Vec3 k3 = lorenz_derivative(Vec3(p.x + k2.x*dt/2, p.y + k2.y*dt/2, p.z + k2.z*dt/2));
    Vec3 k4 = lorenz_derivative(Vec3(p.x + k3.x*dt, p.y + k3.y*dt, p.z + k3.z*dt));
    
    return Vec3(
        p.x + (k1.x + 2*k2.x + 2*k3.x + k4.x) * dt / 6.0f,
        p.y + (k1.y + 2*k2.y + 2*k3.y + k4.y) * dt / 6.0f,
        p.z + (k1.z + 2*k2.z + 2*k3.z + k4.z) * dt / 6.0f
    );
}struct Color {
    float r, g, b;
    Color(float r = 1, float g = 1, float b = 1) : r(r), g(g), b(b) {}
};Color get_color(float t) {
    
    t = fmodf(t, 1.0f);
    
    
    float h = t * 6.0f;  
    int segment = (int)floorf(h);
    float frac = h - segment;
    
    
    frac = frac * frac * (3.0f - 2.0f * frac);
    
    float r, g, b;
    
    switch(segment % 6) {
        case 0:  
            r = 1.0f;
            g = frac;
            b = 0.0f;
            break;
        case 1:  
            r = 1.0f - frac;
            g = 1.0f;
            b = 0.0f;
            break;
        case 2:  
            r = 0.0f;
            g = 1.0f;
            b = frac;
            break;
        case 3:  
            r = 0.0f;
            g = 1.0f - frac;
            b = 1.0f;
            break;
        case 4:  
            r = frac;
            g = 0.0f;
            b = 1.0f;
            break;
        default:  
            r = 1.0f;
            g = 0.0f;
            b = 1.0f - frac;
            break;
    }
    
    return Color(r, g, b);
}class Trajectory {
public:
    std::vector<Vec3> full_path;  
    std::vector<Vec3> visible_trail;  
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
};std::vector<Trajectory> trajectories;
float current_time = 0.0f;
bool paused = false;
int total_steps = 0;
float camera_distance = 120.0f;  
float target_camera_distance = 120.0f;  
const float ZOOM_MIN = 60.0f;
const float ZOOM_MAX = 250.0f;
const float ZOOM_SPEED = 0.05f;  bool recording = false;
FILE* ffmpeg_pipe = nullptr;
std::vector<unsigned char> frame_buffer;
int frame_count = 0;
int record_width = 0;
int record_height = 0;
GLFWwindow* global_window = nullptr;  void start_recording();
void stop_recording();
void capture_frame();void setup_camera() {
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
    
    
    float angle = current_time / 2.0f;
    float cam_x = camera_distance * cosf(angle);
    float cam_y = camera_distance * sinf(angle);
    float cam_z = 25.0f;
    
    gluLookAt(cam_x, cam_y, cam_z,    
              0, 0, 25,                 
              0, 0, 1);                 
}void draw_trail(const Trajectory& traj) {
    if (traj.visible_trail.size() < 2) return;
    
    glLineWidth(4.0f);
    glBegin(GL_LINE_STRIP);
    
    int n = traj.visible_trail.size();
    for (int i = 0; i < n; i++) {
        
        float alpha = (float)i / n;
        alpha = alpha * alpha;  
        
        glColor4f(traj.color.r, traj.color.g, traj.color.b, alpha * 0.9f);
        
        const Vec3& p = traj.visible_trail[i];
        glVertex3f(p.x, p.y, p.z);
    }
    
    glEnd();
}void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    setup_camera();
    
    
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    
    
    
    glDepthMask(GL_FALSE);
    
    
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    
    
    for (const auto& traj : trajectories) {
        draw_trail(traj);
    }
    
    
    glDepthMask(GL_TRUE);
    
    
    if (recording) {
        capture_frame();
    }
}void init_simulation() {
    trajectories.clear();
    trajectories.resize(N_TRAJECTORIES);
    
    total_steps = (int)(TOTAL_TIME / DT);
    
    std::cout << "Computing trajectories..." << std::endl;
    
    for (int i = 0; i < N_TRAJECTORIES; i++) {
        
        Vec3 initial_pos(0.0f, (i + 1.0f) / N_TRAJECTORIES, 0.0f);
        
        trajectories[i].compute(initial_pos, total_steps);
        trajectories[i].color = get_color((float)i / N_TRAJECTORIES);
        
        std::cout << "  Trajectory " << (i + 1) << "/" << N_TRAJECTORIES << std::endl;
    }
    
    std::cout << "Ready!" << std::endl;
}void update() {
    
    camera_distance += (target_camera_distance - camera_distance) * ZOOM_SPEED;
    
    if (paused) return;
    
    bool all_done = true;
    for (auto& traj : trajectories) {
        if (traj.current_step < traj.full_path.size()) {
            traj.update();
            all_done = false;
        }
    }
    
    
    if (all_done) {
        for (auto& traj : trajectories) {
            traj.reset();
        }
        current_time = 0.0f;
    } else {
        current_time += DT;
    }
}void start_recording() {
    if (recording || !global_window) return;
    
    
    glfwGetFramebufferSize(global_window, &record_width, &record_height);
    
    
    time_t now = time(nullptr);
    char filename[256];
    strftime(filename, sizeof(filename), "lorenz_%Y%m%d_%H%M%S.mp4", localtime(&now));
    
    
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
}void stop_recording() {
    if (!recording) return;
    
    recording = false;
    
    if (ffmpeg_pipe) {
        pclose(ffmpeg_pipe);
        ffmpeg_pipe = nullptr;
    }
    
    std::cout << "⏹️  Recording stopped (" << frame_count << " frames)" << std::endl;
    frame_buffer.clear();
    frame_count = 0;
}void capture_frame() {
    if (!recording || !ffmpeg_pipe) return;
    
    
    glReadPixels(0, 0, record_width, record_height, GL_RGB, GL_UNSIGNED_BYTE, frame_buffer.data());
    
    
    std::vector<unsigned char> flipped(frame_buffer.size());
    int row_size = record_width * 3;
    for (int y = 0; y < record_height; y++) {
        memcpy(&flipped[y * row_size], 
               &frame_buffer[(record_height - 1 - y) * row_size], 
               row_size);
    }
    
    
    fwrite(flipped.data(), 1, flipped.size(), ffmpeg_pipe);
    frame_count++;
}void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
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
        case GLFW_KEY_EQUAL:  
        case GLFW_KEY_KP_ADD:
            target_camera_distance -= 8.0f;
            if (target_camera_distance < ZOOM_MIN) target_camera_distance = ZOOM_MIN;
            break;
        case GLFW_KEY_MINUS:  
        case GLFW_KEY_KP_SUBTRACT:
            target_camera_distance += 8.0f;
            if (target_camera_distance > ZOOM_MAX) target_camera_distance = ZOOM_MAX;
            break;
        case GLFW_KEY_0:  
            target_camera_distance = 120.0f;
            break;
        case GLFW_KEY_V:  
            if (action == GLFW_PRESS) {
                if (recording) {
                    stop_recording();
                } else {
                    start_recording();
                }
            }
            break;
    }
}void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    
    target_camera_distance -= (float)yoffset * 10.0f;
    
    
    if (target_camera_distance < ZOOM_MIN) target_camera_distance = ZOOM_MIN;
    if (target_camera_distance > ZOOM_MAX) target_camera_distance = ZOOM_MAX;
}int main() {
    
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    
    glfwWindowHint(GLFW_SAMPLES, 4);  
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
    
    global_window = window;  
    
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSwapInterval(1);  
    
    
    glClearColor(0.04f, 0.04f, 0.04f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    
    
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
    
    
    while (!glfwWindowShouldClose(window)) {
        update();
        render();
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    
    if (recording) {
        stop_recording();
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
