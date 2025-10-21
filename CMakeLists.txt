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
add_library(glad ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include)

# stb_image
add_library(stb_image ${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image/stb_image.cpp)
target_include_directories(stb_image PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image)

add_executable(app main.cpp Shader.cpp)
	
target_link_libraries(app PRIVATE glfw glad stb_image)