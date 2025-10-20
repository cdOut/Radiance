cmake_minimum_required(VERSION 3.15)
project(opengl-test LANGUAGES C CXX)

# GLFW
include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

# GLAD
add_library(glad external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

add_executable(app main.cpp Shader.cpp)
	
target_link_libraries(app PRIVATE glfw glad)