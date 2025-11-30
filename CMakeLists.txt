cmake_minimum_required(VERSION 3.15)
project(opengl-test LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

# GLAD
add_library(glad ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.2
)
FetchContent_MakeAvailable(glm)

# ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.4
)
FetchContent_MakeAvailable(imgui)

# stb_image
add_library(stb_image 
    ${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image/src/stb_image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image/src/stb_image_write.cpp
)
target_include_directories(stb_image PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image/include)

# TinyGLTF
add_library(tiny_gltf 
    ${CMAKE_CURRENT_SOURCE_DIR}/external/tiny_gltf/src/tiny_gltf.cpp
)
target_include_directories(tiny_gltf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/tiny_gltf/include)

add_library(imgui
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw glad)

file(GLOB_RECURSE SRC_FILES src/*.cpp)
add_executable(raycast-thesis MACOSX_BUNDLE WIN32 ${SRC_FILES})
target_include_directories(raycast-thesis PRIVATE include ${tinygltf_SOURCE_DIR})
target_link_libraries(raycast-thesis PRIVATE imgui glm stb_image tiny_gltf)
target_compile_options(stb_image PRIVATE -Wno-deprecated-declarations)

if(APPLE)
    add_custom_command(TARGET raycast-thesis POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:raycast-thesis>/../Resources/assets
    )
endif()

file(COPY assets/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/assets)