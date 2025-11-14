#ifndef APPLICATION_H
#define APPLICATION_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <string>
#include <iostream>
#include <stdexcept>
#include <memory>
#include "editor/core/Scene.h"
#include "editor/mesh/primitives/Sphere.h"
#include "editor/mesh/primitives/Primitive.h"
#include "editor/core/Light.h"

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
            glfwSetWindowSizeLimits(_window, 800, 600, GLFW_DONT_CARE, GLFW_DONT_CARE);

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

            _scene = std::make_unique<Scene>();

            glGetIntegerv(GL_MAX_SAMPLES, &_samples);

            initializeFramebuffer();
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
                float currentFrame = glfwGetTime();
                float deltaTime = currentFrame - _lastTime;
                _lastTime = currentFrame;

                glfwPollEvents();
                processInput(_window, _moveVector);

                _scene->update(deltaTime, _moveVector, _lookDelta, _isRightButtonDown);
                _lookDelta = {0.0f, 0.0f};

                glBindFramebuffer(GL_FRAMEBUFFER, _MSAAFBO);

                glViewport(0, 0, (int)_viewportSize.x, (int)_viewportSize.y);
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                _scene->render(deltaTime);

                glBindFramebuffer(GL_READ_FRAMEBUFFER, _MSAAFBO);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _FBO);
                glBlitFramebuffer(0, 0, _viewportSize.x, _viewportSize.y, 0, 0, _viewportSize.x, _viewportSize.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                renderUI();
                
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
        unsigned int _MSAAFBO, _MSAAColor, _MSAADepth;
        unsigned int _viewportTexture;
        ImVec2 _viewportSize, _lastViewportSize;
        int _samples;

        glm::vec2 _moveVector;
        glm::vec2 _lookDelta{0.0f};
        glm::vec2 _lastMousePos{0.0f};
        bool _firstMouse = true;
        bool _isViewportHovered = false;
        bool _isRightButtonDown = false;

        float _lastTime = 0.0f;

        std::unique_ptr<Scene> _scene;

        void initializeFramebuffer() {
            glGenFramebuffers(1, &_FBO);
            glBindFramebuffer(GL_FRAMEBUFFER, _FBO);

            glGenTextures(1, &_viewportTexture);
            glGenRenderbuffers(1, &_RBO);

            resizeFramebuffer(800, 600);

            glGenFramebuffers(1, &_MSAAFBO);
            glBindFramebuffer(GL_FRAMEBUFFER, _MSAAFBO);

            glGenRenderbuffers(1, &_MSAAColor);
            glGenRenderbuffers(1, &_MSAADepth);

            resizeMSAAFramebuffer(800, 600);
                     
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                throw std::runtime_error("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");
            
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void resizeFramebuffer(int width, int height) {
            glBindFramebuffer(GL_FRAMEBUFFER, _FBO);

            glBindTexture(GL_TEXTURE_2D, _viewportTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
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

        void resizeMSAAFramebuffer(int width, int height) {
            glBindFramebuffer(GL_FRAMEBUFFER, _MSAAFBO);

            glBindRenderbuffer(GL_RENDERBUFFER, _MSAAColor);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, _samples, GL_RGBA8, width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _MSAAColor);

            glBindRenderbuffer(GL_RENDERBUFFER, _MSAADepth);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, _samples, GL_DEPTH24_STENCIL8, width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _MSAADepth);

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

            if (!app->_isRightButtonDown) return;

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
                    app->_firstMouse = true;
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

                        for (const auto& primitive : primitiveList) {
                            if (ImGui::MenuItem(primitive.name)) {
                                Entity* selected = _scene->getSelectedEntity();
                                if (selected)
                                    selected->setIsSelected(false);

                                Entity* entity = primitive.create(_scene.get());
                                _scene.get()->setSelectedEntity(entity);
                                entity->setIsSelected(true);
                                entity->setName(_scene->generateUniqueName(primitive.name));
                            }
                        }

                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Light")) {
                        for (int i = 0; i < static_cast<int>(LightType::END); i++) {
                            LightType lightType = static_cast<LightType>(i);
                            if (ImGui::MenuItem(getLightTypeName(lightType).c_str())) {
                                Entity* selected = _scene->getSelectedEntity();
                                if (selected)
                                    selected->setIsSelected(false);

                                Entity* entity = _scene->createEntity<Light>(lightType);
                                _scene.get()->setSelectedEntity(entity);
                                entity->setIsSelected(true);
                                entity->setName(_scene->generateUniqueName(getLightTypeName(lightType).c_str()));
                            }
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }
                ImGui::MenuItem("Render");
                ImGui::MenuItem("Help");
                ImGui::EndMainMenuBar();
            }

            float menuBarHeight = ImGui::GetFrameHeight();
            float leftWidth = _width - 250.0f;
            float rightWidth = _width - leftWidth;
            float panelHeight = _height - menuBarHeight;
            float topHeight = panelHeight * 0.4f;

            ImGui::SetNextWindowPos(ImVec2(0, menuBarHeight));
            ImGui::SetNextWindowSize(ImVec2(leftWidth, panelHeight));
            ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

            _viewportSize = ImGui::GetContentRegionAvail();
            if (_viewportSize.x != _lastViewportSize.x || _viewportSize.y != _lastViewportSize.y) {
                resizeFramebuffer(_viewportSize.x, _viewportSize.y);
                resizeMSAAFramebuffer(_viewportSize.x, _viewportSize.y);
                _lastViewportSize = _viewportSize;

                _scene->getCamera()->setAspect(_viewportSize.x / _viewportSize.y);
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

            for (auto& e : _scene->getEntities()) {
                if (e.get() == _scene->getCamera()) continue;
                if (e.get() == _scene->getGrid()) continue;

                bool isSelected = (e.get() == _scene->getSelectedEntity());
                if (ImGui::Selectable(e->getName().c_str(), isSelected)) {
                    Entity* selected = _scene->getSelectedEntity();
                    if (selected)
                        selected->setIsSelected(false);
                    
                    _scene->setSelectedEntity(e.get());
                    e.get()->setIsSelected(true);
                }
            }

            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(leftWidth, menuBarHeight + topHeight));
            ImGui::SetNextWindowSize(ImVec2(rightWidth, panelHeight - topHeight));
            ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

            Entity* selected = _scene->getSelectedEntity();
            if (selected) {
                char nameBuffer[64];
                std::strncpy(nameBuffer, selected->getName().c_str(), sizeof(nameBuffer));

                if (ImGui::CollapsingHeader("Name", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::PushItemWidth(-1);
                    if (ImGui::InputText("##Name", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                    {
                        std::string name = nameBuffer;
                        if (nameBuffer != selected->getName() && _scene->isNameTaken(nameBuffer)) {
                            name = _scene->generateUniqueName(nameBuffer);
                        }

                        if (std::strlen(nameBuffer) > 0)
                            selected->setName(name);
                    }
                    ImGui::PopItemWidth();
                }

                Transform& transform = selected->getTransform();

                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::DragFloat3("Position", glm::value_ptr(transform.position), 0.1f);
                    ImGui::DragFloat3("Rotation", glm::value_ptr(transform.rotation), 0.1f);
                    ImGui::DragFloat3("Scale",    glm::value_ptr(transform.scale),    0.1f);
                }

                ImGui::Separator();
                if (ImGui::Button("Delete Entity", ImVec2(-FLT_MIN, 0))) {
                    _scene->removeEntity(selected);
                }
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
                if (selected)
                    selected->setIsSelected(false);
                _scene->setSelectedEntity(nullptr);
            }

            ImGui::End();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
};

#endif