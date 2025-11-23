cmake_minimum_required(VERSION 3.10)
project(LorenzButterfly)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Add executable
add_executable(lorenz_butterfly lorenz_butterfly.cpp)

# Link libraries
target_link_libraries(lorenz_butterfly
    OpenGL::GL
    OpenGL::GLU
    GLEW::GLEW
    glfw
    glm::glm
)

# Set optimization flags for Release build
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

# macOS specific settings
if(APPLE)
    target_link_libraries(lorenz_butterfly "-framework OpenGL" "-framework Cocoa" "-framework IOKit")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")

