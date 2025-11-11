#ifndef APPLICATION_H
#define APPLICATION_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <string>
#include <iostream>

class Application {
    public:
        Application(int width, int height, const char* title) : _width(width), _height(height), _title(title) {
            glfwInit();
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            #ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
            #endif

            _window = glfwCreateWindow(_width, _height, _title, NULL, NULL);
            if (_window == NULL) {
                glfwTerminate();
                throw std::runtime_error("Failed to create GLFW window");
            }

            glfwMakeContextCurrent(_window);

            if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
                throw std::runtime_error("Failed to initialize GLAD");
            }

            glfwSwapInterval(1);
        }

        ~Application() {
            glfwTerminate();
        }

        void run() {
            while (!glfwWindowShouldClose(_window)) {
                glfwSwapBuffers(_window);
                glfwPollEvents();
            }
        }
    private:
        GLFWwindow* _window;
        int _width;
        int _height;
        const char* _title;
};

#endif