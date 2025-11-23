# Lorenz Butterfly (Lorenz Attractor)

Visualization of the Lorenz attractor using OpenGL. Zooming in/out, replay, changing initial condition, video recording. 


https://github.com/user-attachments/assets/d611454f-4740-408a-b27b-ff6316ce1cf6


## Quick Start

### Prerequisites

**macOS:**
```bash
brew install glfw glm ffmpeg
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install libglfw3-dev libglm-dev ffmpeg
```

### Build & Run

```bash
make
./lorenz_butterfly
```

## 🎮 Controls

| Key | Action |
|-----|--------|
| **SPACE** | Pause/Resume animation |
| **R** | Restart from beginning |
| **+** or **=** | Zoom in |
| **-** | Zoom out |
| **0** | Reset zoom to default |
| **V** | Start/Stop video recording |
| **Mouse Wheel** | Zoom in/out |
| **ESC** or **Q** | Quit |

## 📹 Recording Videos

1. Press **V** to start recording
2. The animation continues normally - zoom, rotate, etc.
3. Press **V** again to stop
4. Video saves as `lorenz_YYYYMMDD_HHMMSS.mp4`

**Output specs:**
- Resolution: Native framebuffer (e.g., 3840×2160 on Retina)
- Frame rate: monitor FPS
- Codec: H.264 (CRF 18 - near lossless quality)
- Compatible with all video players

## 🔬 The Mathematics

The Lorenz system is a set of three ordinary differential equations:

```
dx/dt = σ(y - x)
dy/dt = x(ρ - z) - y
dz/dt = xy - βz
```

With the classic chaotic parameters:
- **σ** (sigma) = 10
- **ρ** (rho) = 28
- **β** (beta) = 8/3


## Customization

Edit these constants in `lorenz_butterfly.cpp`:

```cpp
const int N_TRAJECTORIES = 40;        // Number of trajectories (8-80)
const int MAX_TRAIL_LENGTH = 1000;    // Trail length in points
const float DT = 0.005f;              // Time step (smaller = smoother)
const float ZOOM_SPEED = 0.05f;       // Zoom smoothness (0.01-0.3)
```

**Lorenz parameters:**
```cpp
const float SIGMA = 10.0f;
const float RHO = 28.0f;
const float BETA = 8.0f / 3.0f;
```
