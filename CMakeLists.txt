cmake_minimum_required(VERSION 3.10)
project(gravity_sim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenGL and GLFW
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# Include directories for GLAD + GLFW
include_directories(include)
include_directories(${GLFW_INCLUDE_DIRS})

# Add executable (glad.c must be compiled with your project)
add_executable(gravity_sim
    src/main.cpp
    src/Shader.cpp
    src/Camera.cpp
    src/glad.c
)

# Link libraries
target_link_libraries(gravity_sim
    ${OPENGL_LIBRARIES}
    glfw
    dl       # needed for dynamic loading
    pthread  # needed for GLFW
)
