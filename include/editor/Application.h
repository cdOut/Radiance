#ifndef APPLICATION_H
#define APPLICATION_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <string>
#include <iostream>
#include <stdexcept>

#include <mesh/Mesh.h>
#include <mesh/Sphere.h>
#include <Camera.h>
#include <Shader.h>
#include <Grid.h>

class Application {
    public:
        Application(int width, int height, const char* title) : _width(width), _height(height), _title(title) {
            if (!glfwInit()) {
                throw std::runtime_error("Failed to initialize GLFW");
            }

            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            #ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
            #endif

            _window = glfwCreateWindow(_width, _height, _title, nullptr, nullptr);
            if (!_window) {
                glfwTerminate();
                throw std::runtime_error("Failed to create GLFW window");
            }

            glfwMakeContextCurrent(_window);
            glfwSwapInterval(1);
            glfwSetWindowUserPointer(_window, this);

            glfwSetFramebufferSizeCallback(_window, framebufferSizeCallback);
            glfwSetWindowSizeCallback(_window, windowSizeCallback);
            glfwSetCursorPosCallback(_window, mouseCallback); 
            glfwSetMouseButtonCallback(_window, mouseButtonCallback);

            if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
                glfwDestroyWindow(_window);
                glfwTerminate();
                throw std::runtime_error("Failed to initialize GLAD");
            }

            int fbWidth, fbHeight;
            glfwGetFramebufferSize(_window, &fbWidth, &fbHeight);
            glViewport(0, 0, fbWidth, fbHeight);

            glEnable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            ImGui::StyleColorsDark();

            ImGui_ImplGlfw_InitForOpenGL(_window, true);
            ImGui_ImplOpenGL3_Init("#version 330");

            initializeFramebuffer();

            _shader = Shader("assets/shaders/baseShader.vs", "assets/shaders/litShader.fs");
            _gridShader = Shader("assets/shaders/grid.vs", "assets/shaders/grid.fs");
            _camera = Camera(90.0f, 16.0f/9.0f, 0.1f, 100.0f);
            _grid = std::make_unique<Grid>();
            _mesh = Mesh::Create<Sphere>();
        }

        ~Application() {
            glDeleteFramebuffers(1, &_FBO);
            glDeleteRenderbuffers(1, &_RBO);

            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();

            glfwDestroyWindow(_window);
            glfwTerminate();
        }

        void run() {
            while (!glfwWindowShouldClose(_window)) {
                glfwPollEvents();
                processInput(_window, _moveVector);

                float currentFrame = glfwGetTime();
                _deltaTime = currentFrame - _lastTime;
                _lastTime = currentFrame;

                glBindFramebuffer(GL_FRAMEBUFFER, _FBO);

                glViewport(0, 0, (int)_viewportSize.x, (int)_viewportSize.y);
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                _shader.use();
                _shader.setMat4("view", _camera.getViewMatrix());
                _shader.setMat4("projection", _camera.getProjectionMatrix());

                _shader.setVec3("color", {1.0f, 0.5f, 0.31f});
                _shader.setVec3("lightColor", {1.0f, 1.0f, 1.0f});
                _shader.setVec3("lightPos", {0.0f, 1.0f, 2.0f});
                _shader.setVec3("viewPos", {0.0f, 0.0f, 3.0f});

                _gridShader.use();
                _gridShader.setMat4("view", _camera.getViewMatrix());
                _gridShader.setMat4("projection", _camera.getProjectionMatrix());

                _grid->handleCameraPos(_camera.getTransform().position);

                _grid->render(_gridShader);
                _mesh->render(_shader);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                renderUI();

                if (_isRightButtonDown) {
                    _camera.handleMove(_moveVector, _deltaTime);
                    _camera.handleLook(_lookDelta);
                }
                _lookDelta = {0.0f, 0.0f};
                
                glfwSwapBuffers(_window);
            }
        }
    private:
        GLFWwindow* _window;
        int _width;
        int _height;
        const char* _title;

        float _aspect = 16.0f / 9.0f;

        unsigned int _FBO, _RBO;
        unsigned int _viewportTexture;
        ImVec2 _viewportSize, _lastViewportSize;

        Shader _shader;
        Shader _gridShader;
        Camera _camera;
        std::unique_ptr<Mesh> _mesh;
        std::unique_ptr<Grid> _grid;

        glm::vec2 _moveVector;
        glm::vec2 _lookDelta{0.0f};
        glm::vec2 _lastMousePos{0.0f};
        bool _firstMouse = true;
        bool _isViewportHovered;
        bool _isRightButtonDown;

        float _deltaTime = 0.0f, _lastTime = 0.0f;

        void initializeFramebuffer() {
            glGenFramebuffers(1, &_FBO);
            glBindFramebuffer(GL_FRAMEBUFFER, _FBO);

            glGenTextures(1, &_viewportTexture);
            glGenRenderbuffers(1, &_RBO);

            resizeFramebuffer(800, 600);
                     
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                throw std::runtime_error("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");
            
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void resizeFramebuffer(int width, int height) {
            glBindFramebuffer(GL_FRAMEBUFFER, _FBO);

            glBindTexture(GL_TEXTURE_2D, _viewportTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _viewportTexture, 0);

            glBindRenderbuffer(GL_RENDERBUFFER, _RBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _RBO);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        static void framebufferSizeCallback(GLFWwindow* window, int fbWidth, int fbHeight) {
            glViewport(0, 0, fbWidth, fbHeight);
        }

        static void windowSizeCallback(GLFWwindow* window, int width, int height) {
            auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
            app->_width = width;
            app->_height = height;
        }

        static void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
            auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

            if (app->_firstMouse) {
                app->_lastMousePos = {xpos, ypos};
                app->_firstMouse = false;
            }

            float xOffset = xpos - app->_lastMousePos.x;
            float yOffset = app->_lastMousePos.y - ypos;

            app->_lastMousePos = {xpos, ypos};
            app->_lookDelta = {xOffset, yOffset};
        }

        static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
            auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

            if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                if (action == GLFW_PRESS && app->_isViewportHovered) {
                    app->_isRightButtonDown = true;
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                } else if (action == GLFW_RELEASE) {
                    app->_isRightButtonDown = false;
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }
            }
        }

        void processInput(GLFWwindow *window, glm::vec2& moveVector) {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, true);
            }

            moveVector = {0.0f, 0.0f};

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveVector.y += 1.0f;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveVector.y -= 1.0f;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveVector.x -= 1.0f;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveVector.x += 1.0f;

            if (glm::length(moveVector) > 1.0f)
                moveVector = glm::normalize(moveVector);
        }

        void renderUI() {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("Scene")) {
                    ImGui::MenuItem("New scene");
                    ImGui::MenuItem("Load scene");
                    ImGui::MenuItem("Save scene");
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Add")) {
                    if (ImGui::BeginMenu("Mesh")) {
                        ImGui::MenuItem("Sphere");
                        ImGui::MenuItem("Plane");
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Light")) {
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }
                ImGui::MenuItem("Render");
                ImGui::MenuItem("Help");
                ImGui::EndMainMenuBar();
            }

            float menuBarHeight = ImGui::GetFrameHeight();
            float leftWidth = _width - 200.0f;
            float rightWidth = _width - leftWidth;
            float panelHeight = _height - menuBarHeight;
            float topHeight = panelHeight * 0.4f;

            ImGui::SetNextWindowPos(ImVec2(0, menuBarHeight));
            ImGui::SetNextWindowSize(ImVec2(leftWidth, panelHeight));
            ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

            _viewportSize = ImGui::GetContentRegionAvail();
            if (_viewportSize.x != _lastViewportSize.x || _viewportSize.y != _lastViewportSize.y) {
                resizeFramebuffer(_viewportSize.x, _viewportSize.y);
                _lastViewportSize = _viewportSize;

                _camera.setAspect(_viewportSize.x / _viewportSize.y);
            }

            ImGui::Image((ImTextureID)(intptr_t)_viewportTexture, _viewportSize, ImVec2(0, 1), ImVec2(1, 0));
            _isViewportHovered = ImGui::IsItemHovered();

            ImVec2 imageMin = ImGui::GetItemRectMin();
            ImVec2 imageMax = ImGui::GetItemRectMax();
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            float aspectWidth = _viewportSize.x;
            float aspectHeight = _viewportSize.x / _aspect;
            if (aspectHeight > _viewportSize.y) {
                aspectWidth = _viewportSize.y * _aspect;
                aspectHeight = _viewportSize.y;
            }

            float overlayWidth = (_viewportSize.x - aspectWidth) * 0.5f;
            float overlayHeight = (_viewportSize.y - aspectHeight) * 0.5f;

            ImU32 overlayColor = IM_COL32(16, 16, 16, 192);

            drawList->AddRectFilled(ImVec2(imageMin.x, imageMin.y), ImVec2(imageMin.x + overlayWidth, imageMax.y), overlayColor);
            drawList->AddRectFilled(ImVec2(imageMax.x - overlayWidth, imageMin.y), ImVec2(imageMax.x, imageMax.y), overlayColor);
            drawList->AddRectFilled(ImVec2(imageMin.x + overlayWidth, imageMin.y), ImVec2(imageMax.x - overlayWidth, imageMin.y + overlayHeight), overlayColor);
            drawList->AddRectFilled(ImVec2(imageMin.x + overlayWidth, imageMax.y - overlayHeight), ImVec2(imageMax.x - overlayWidth, imageMax.y), overlayColor);

            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(leftWidth, menuBarHeight));
            ImGui::SetNextWindowSize(ImVec2(rightWidth, topHeight));
            ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(leftWidth, menuBarHeight + topHeight));
            ImGui::SetNextWindowSize(ImVec2(rightWidth, panelHeight - topHeight));
            ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::End();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
};

#endif