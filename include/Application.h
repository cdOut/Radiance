#ifndef APPLICATION_H
#define APPLICATION_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "tinyfiledialogs.h"

#include <string>
#include <iostream>
#include <stdexcept>
#include <memory>
#include "editor/Scene.h"
#include "editor/entity/mesh/primitives/Primitive.h"
#include "editor/entity/light/LightList.h"

#include "raytracer/Raytracer.h"
#include <thread>
#include <atomic>
#include <chrono>
#include "editor/Exporter.h"
#include "editor/MeshImporter.h"
#include "editor/Importer.h"
#include <mutex>
#include "ImGuizmo.h"

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

            GLFWimage icon;
            int channels;
            icon.pixels = stbi_load("assets/textures/icon.png", &icon.width, &icon.height, &channels, 4);
            if (icon.pixels) {
                glfwSetWindowIcon(_window, 1, &icon);
                stbi_image_free(icon.pixels);
            }

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
            glEnable(GL_STENCIL_TEST);

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            ImGui::GetIO().IniFilename = nullptr;
            ImGui::StyleColorsDark();

            ImGui_ImplGlfw_InitForOpenGL(_window, true);
            ImGui_ImplOpenGL3_Init("#version 330");

            _scene = std::make_unique<Scene>();

            glGetIntegerv(GL_MAX_SAMPLES, &_samples);

            initializeFramebuffer();

            glGenTextures(1, &_renderId);
            glBindTexture(GL_TEXTURE_2D, _renderId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);

            _iconGrid = loadIcon("assets/textures/grid.png");
            _iconTranslate = loadIcon("assets/textures/translate.png");
            _iconRotate = loadIcon("assets/textures/rotate.png");
            _iconScale = loadIcon("assets/textures/scale.png");
        }

        ~Application() {
            glDeleteFramebuffers(1, &_FBO);
            glDeleteRenderbuffers(1, &_RBO);

            _raytraceInProgress = false;

            if (_raytraceThread.joinable())
                _raytraceThread.join();

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
                glm::vec3 color = _scene->getSkyboxColor();
                glm::vec3 gammaColor = glm::pow(color, glm::vec3(1.0f / 2.2f));
                glClearColor(gammaColor.r, gammaColor.g, gammaColor.b, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

                _scene->renderShadowPass((int)_viewportSize.x, (int)_viewportSize.y);

                glBindFramebuffer(GL_FRAMEBUFFER, _MSAAFBO);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

                _scene->render(deltaTime);

                glBindFramebuffer(GL_READ_FRAMEBUFFER, _MSAAFBO);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _FBO);
                glBlitFramebuffer(0, 0, _viewportSize.x, _viewportSize.y, 0, 0, _viewportSize.x, _viewportSize.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                glBindFramebuffer(GL_FRAMEBUFFER, _selectFBO);

                glViewport(0, 0, (int)_viewportSize.x, (int)_viewportSize.y);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

                _scene->renderGpuSelect();

                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                if (_scanlineUpdated) {
                    std::lock_guard<std::mutex> lock(_previewMutex);
                    int height = _renderWidth * 9 / 16;

                    glBindTexture(GL_TEXTURE_2D, _renderId);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _renderWidth, height, GL_RGB, GL_UNSIGNED_BYTE, _renderData.data());
                    glBindTexture(GL_TEXTURE_2D, 0);

                    _scanlineUpdated = false;
                }

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
        ImVec2 _viewportSize, _lastViewportSize, _viewportPos;
        int _samples;

        unsigned int _selectFBO, _selectColor, _selectDepth;

        glm::vec2 _moveVector;
        glm::vec2 _lookDelta{0.0f};
        glm::vec2 _lastMousePos{0.0f};
        bool _firstMouse = true;
        bool _isViewportHovered = false;
        bool _isRightButtonDown = false;

        float _lastTime = 0.0f;

        std::thread _raytraceThread;
        std::atomic<bool> _raytraceInProgress = false;
        std::atomic<bool> _raytraceFinished = false;
        unsigned int _renderId = 0;
        std::vector<unsigned char> _renderData;
        std::atomic<bool> _scanlineUpdated = false;
        int _scanlineToUpload = -1;
        std::mutex _previewMutex;
        std::chrono::duration<double> _raytraceDuration{0.0};

        std::unique_ptr<Scene> _scene;

        bool _openSkyboxColorPopup = false;
        bool _openRenderPopup = false;
        bool _openCameraPopup = false;

        int _renderWidth = 120;
        int _samplesPerPixel = 100;
        int _maxDepth = 50;

        unsigned int _saveFBO = 0, _saveColor = 0;

        ImGuizmo::OPERATION _gizmoOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE _gizmoMode = ImGuizmo::WORLD;

        unsigned int _iconGrid = 0, _iconTranslate = 0, _iconRotate = 0, _iconScale = 0;
        bool _showAddContextMenu = false, _showDeleteContextMenu = false;
        ImVec2 _contextMenuPos;

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

            glGenFramebuffers(1, &_selectFBO);
            glBindFramebuffer(GL_FRAMEBUFFER, _selectFBO);

            glGenTextures(1, &_selectColor);
            glGenRenderbuffers(1, &_selectDepth);

            resizeSelectFramebuffer(800, 600);
                     
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                throw std::runtime_error("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");
            
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void initializeSaveFramebuffer() {
            if (_saveFBO == 0) {
                glGenFramebuffers(1, &_saveFBO);
                glGenTextures(1, &_saveColor);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, _saveFBO);

            glBindTexture(GL_TEXTURE_2D, _saveColor);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _viewportSize.x, _viewportSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _saveColor, 0);

            GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, drawBuffers);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                throw std::runtime_error("ERROR::FRAMEBUFFER:: Save framebuffer is not complete!");

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void saveViewport() {
            if (_viewportSize.x <= 0 || _viewportSize.y <= 0)
                return;

            initializeSaveFramebuffer();

            glBindFramebuffer(GL_READ_FRAMEBUFFER, _MSAAFBO);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _saveFBO);

            glBlitFramebuffer(0, 0, (int) _viewportSize.x, (int) _viewportSize.y, 0, 0, (int) _viewportSize.x, (int) _viewportSize.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);

            std::vector<unsigned char> data(_viewportSize.x * _viewportSize.y * 4);
            glBindFramebuffer(GL_FRAMEBUFFER, _saveFBO);
            glReadPixels(0, 0, _viewportSize.x, _viewportSize.y, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            stbi_flip_vertically_on_write(true);
            stbi_write_png("viewport.png", _viewportSize.x, _viewportSize.y, 4, data.data(), _viewportSize.x * 4);
        }

        void resizeFramebuffer(int width, int height) {
            glBindFramebuffer(GL_FRAMEBUFFER, _FBO);

            glBindTexture(GL_TEXTURE_2D, _viewportTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
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

        void resizeSelectFramebuffer(int width, int height) {
            glBindFramebuffer(GL_FRAMEBUFFER, _selectFBO);

            glBindTexture(GL_TEXTURE_2D, _selectColor);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _selectColor, 0);

            glBindRenderbuffer(GL_RENDERBUFFER, _selectDepth);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _selectDepth);

            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glReadBuffer(GL_COLOR_ATTACHMENT0);

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

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && _isViewportHovered) {
                Entity* sel = _scene->getSelectedEntity();
                bool gizmoActive = sel && sel != _scene->getCamera() && sel != _scene->getGrid();
                if (gizmoActive && ImGuizmo::IsOver()) return;
                if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopup)) return;
                
                ImVec2 mousePos = ImGui::GetMousePos();
                ImVec2 localPos = {mousePos.x - _viewportPos.x, mousePos.y - _viewportPos.y};

                if (localPos.x >= 0 && localPos.y >= 0 && localPos.x < _viewportSize.x && localPos.y < _viewportSize.y) {
                    unsigned char pixel[4];

                    glBindFramebuffer(GL_FRAMEBUFFER, _selectFBO);
                    glReadPixels((int)localPos.x, (int)(_viewportSize.y - localPos.y), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);

                    Entity* selected = nullptr, *lastSelected = nullptr;
                    auto& entities = _scene->getEntities();

                    unsigned int id =  (pixel[0]) | (pixel[1] << 8) | (pixel[2] << 16);

                    if (!id)
                        return;

                    if (entities.find(id) != entities.end()) {
                        if (_scene->getSelectedEntity())
                            _scene->getSelectedEntity()->setIsSelected(false);

                        if (entities.at(id).get())
                            _scene->setSelectedEntity(entities.at(id).get());
                            entities.at(id).get()->setIsSelected(true);
                    }
                }
            }
        }

        void renderEditor() {
            float menuBarHeight = ImGui::GetFrameHeight();
            float leftWidth = _width - 260.0f;
            float panelHeight = _height - menuBarHeight;
            float topHeight = panelHeight * 0.4f;

            ImGui::BeginChild("Viewport", ImVec2(leftWidth, 0), true, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

            _viewportSize = ImGui::GetContentRegionAvail();
            if (_viewportSize.x != _lastViewportSize.x || _viewportSize.y != _lastViewportSize.y) {
                resizeFramebuffer(_viewportSize.x, _viewportSize.y);
                resizeMSAAFramebuffer(_viewportSize.x, _viewportSize.y);
                resizeSelectFramebuffer(_viewportSize.x, _viewportSize.y);
                _lastViewportSize = _viewportSize;

                _scene->getCamera()->setAspect(_viewportSize.x / _viewportSize.y);
            }

            ImGui::Image((ImTextureID)(intptr_t)_viewportTexture, _viewportSize, ImVec2(0, 1), ImVec2(1, 0));
            _viewportPos = ImGui::GetItemRectMin();
            _isViewportHovered = ImGui::IsItemHovered();

            ImVec2 imageMin = ImGui::GetItemRectMin();
            ImVec2 imageMax = ImGui::GetItemRectMax();

            ImVec2 toolbarSize = ImVec2(4 * 36 + 8, 32);

            ImGui::SetCursorScreenPos(ImVec2(
                _viewportPos.x + _viewportSize.x - toolbarSize.x - 12,
                _viewportPos.y + 12
            ));

            ImGui::BeginChild("GizmoToolbar", toolbarSize, false,
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoNav |
                ImGuiWindowFlags_NoMove
            );

            auto iconToggleButton = [&](unsigned int iconId, bool active, const char* tooltip) -> bool {
                ImVec4 tint = active ? ImVec4(1, 1, 1, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
                if (active) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(16/255.0f, 16/255.0f, 16/255.0f, 192/255.0f));
                }

                bool clicked = ImGui::ImageButton(tooltip, (ImTextureID)(intptr_t)iconId, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), tint);
                ImGui::PopStyleColor();
                ImGui::PopStyleVar();

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", tooltip);

                return clicked;
            };

            if (iconToggleButton(_iconGrid, _scene->_showGrid, "Toggle Grid"))
                _scene->_showGrid = !_scene->_showGrid;
            ImGui::SameLine();
            if (iconToggleButton(_iconTranslate, _gizmoOperation == ImGuizmo::TRANSLATE, "Translate (G)"))
                _gizmoOperation = ImGuizmo::TRANSLATE;
            ImGui::SameLine();
            if (iconToggleButton(_iconRotate, _gizmoOperation == ImGuizmo::ROTATE, "Rotate (R)"))
                _gizmoOperation = ImGuizmo::ROTATE;
            ImGui::SameLine();
            if (iconToggleButton(_iconScale, _gizmoOperation == ImGuizmo::SCALE, "Scale (S)"))
                _gizmoOperation = ImGuizmo::SCALE;

            ImGui::EndChild();

            if (_isViewportHovered && !ImGui::IsAnyItemActive()) {
                if (!_isRightButtonDown) {
                    if (ImGui::IsKeyPressed(ImGuiKey_A)) {
                        _contextMenuPos = ImGui::GetMousePos();
                        _showAddContextMenu = true;
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_X)) {
                        _contextMenuPos = ImGui::GetMousePos();
                        _showDeleteContextMenu = true;
                    }
                    
                    if (ImGui::IsKeyPressed(ImGuiKey_G)) _gizmoOperation = ImGuizmo::TRANSLATE;
                    if (ImGui::IsKeyPressed(ImGuiKey_R)) _gizmoOperation = ImGuizmo::ROTATE;
                    if (ImGui::IsKeyPressed(ImGuiKey_S)) _gizmoOperation = ImGuizmo::SCALE;
                }
            }

            Entity* selected = _scene->getSelectedEntity();
            if (selected && selected != _scene->getCamera() && selected != _scene->getGrid()) {
                ImGuizmo::SetOrthographic(false);
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(_viewportPos.x, _viewportPos.y, _viewportSize.x, _viewportSize.y);

                glm::mat4 view = _scene->getCamera()->getViewMatrix();
                glm::mat4 proj = _scene->getCamera()->getProjectionMatrix();

                glm::mat4 model = selected->getModelMatrix();
                glm::mat4 delta(1.0f);

                if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), _gizmoOperation, ImGuizmo::WORLD, glm::value_ptr(model), glm::value_ptr(delta))) {
                    Transform& t = selected->getTransform();

                    if (_gizmoOperation == ImGuizmo::TRANSLATE) {
                        glm::vec3 dp, dr, ds;
                        ImGuizmo::DecomposeMatrixToComponents(
                            glm::value_ptr(delta),
                            glm::value_ptr(dp),
                            glm::value_ptr(dr),
                            glm::value_ptr(ds)
                        );

                        t.position += dp;
                    } else if (_gizmoOperation == ImGuizmo::ROTATE) {
                        glm::quat dq = glm::quat_cast(delta);

                        float angle = glm::degrees(glm::angle(dq));
                        glm::vec3 axis = glm::axis(dq);

                        axis = glm::normalize(axis);

                        if (fabs(axis.x) > 0.9f)
                            t.rotation.x += angle * (axis.x > 0 ? 1.f : -1.f);
                        else if (fabs(axis.y) > 0.9f)
                            t.rotation.y += angle * (axis.y > 0 ? 1.f : -1.f);
                        else if (fabs(axis.z) > 0.9f)
                            t.rotation.z += angle * (axis.z > 0 ? 1.f : -1.f);
                    } else if (_gizmoOperation == ImGuizmo::SCALE) {
                        glm::vec3 dp, dr, ds;
                        ImGuizmo::DecomposeMatrixToComponents(
                            glm::value_ptr(delta),
                            glm::value_ptr(dp),
                            glm::value_ptr(dr),
                            glm::value_ptr(ds)
                        );

                        t.scale *= ds;
                    }
                }
            }

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

            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginGroup();

            ImGui::TextUnformatted("Hierarchy");
            ImGui::Separator();

            ImGui::BeginChild("Hierarchy", ImVec2(0, topHeight), true, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

            for (const auto& [_, e] : _scene->getEntities()) {
                if (e.get() == _scene->getCamera()) continue;
                if (e.get() == _scene->getGrid()) continue;

                bool isSelected = (e.get() == _scene->getSelectedEntity());
                if (ImGui::Selectable(e->getName().c_str(), isSelected)) {
                    selected = _scene->getSelectedEntity();
                    if (selected)
                        selected->setIsSelected(false);
                    
                    _scene->setSelectedEntity(e.get());
                    e.get()->setIsSelected(true);
                }
            }

            ImGui::EndChild();

            ImGui::TextUnformatted("Inspector");
            ImGui::Separator();

            ImGui::BeginChild("Inspector", ImVec2(0, 0), true, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

            selected = _scene->getSelectedEntity();
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

                Mesh* mesh = dynamic_cast<Mesh*>(selected);
                if (mesh) {
                    Material& material = mesh->getMaterial();

                    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::ColorEdit3("Albedo", glm::value_ptr(material.albedo));
                        ImGui::SliderFloat("Metallic", &material.metallic, 0.0f, 1.0f);
                        ImGui::SliderFloat("Roughness", &material.roughness, 0.0f, 1.0f);
                    }
                }

                Light* light = dynamic_cast<Light*>(selected);
                if (light) {
                    if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::ColorEdit3("Color", glm::value_ptr(light->getColor()));
                        ImGui::SliderFloat("Intensity", &light->getIntensity(), 0.0f, 10.0f);

                        SpotLight* spot = dynamic_cast<SpotLight*>(light);
                        if (spot) {
                            ImGui::SliderFloat("Size", &spot->getSize(), 0.0f, 180.0f);
                            ImGui::SliderFloat("Blend", &spot->getBlend(), 0.0f, 1.0f);
                        }
                    }
                }

                ImGui::Separator();
                if (ImGui::Button("Delete Entity", ImVec2(-FLT_MIN, 0))) {
                    _scene->removeEntity(selected);
                }
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
                if (!ImGuizmo::IsOver() && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopup)) {
                    if (selected)
                        selected->setIsSelected(false);
                    _scene->setSelectedEntity(nullptr);
                }
            }

            ImGui::EndChild();

            ImGui::EndGroup();

            ImGui::SameLine();
            ImGui::Dummy(ImVec2(10.0f, 0.0f));
        }

        void renderRaytracer() {
            ImGuiStyle& style = ImGui::GetStyle();
            float infoHeight = ImGui::GetFrameHeight() + style.WindowPadding.y * 2;
            ImGui::BeginChild("Render info", ImVec2(0, infoHeight), true, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            
            if (ImGui::BeginTable("table", 2, ImGuiTableFlags_SizingStretchSame)) {
                ImGui::TableNextColumn();
                std::string status = _raytraceInProgress ? "RENDERING" : "COMPLETED";
                if (_renderData.empty() && !_raytraceInProgress)
                    status = "-";
                ImGui::Text("Status: %s", status.c_str());

                ImGui::TableNextColumn();
                if (_raytraceInProgress) {
                    int scanline = RayCamera::currentScanline.load();
                    int scansize = RayCamera::scanlineSize.load();
                    float progress = 1.0f - (float)scanline / (float)(scansize - 1);
                    ImGui::Text("Progress: ");
                    ImGui::SameLine();
                    float barHeight = ImGui::GetTextLineHeight();
                    ImVec4 blue = ImGui::GetStyleColorVec4(ImGuiCol_SliderGrabActive); 
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, blue);
                    ImGui::ProgressBar(progress, ImVec2(-1, barHeight));
                    ImGui::PopStyleColor();
                } else {
                    ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize("Time elapsed: 00:00:00").x);
                    if (!_renderData.empty()) {
                        int hours   = static_cast<int>(_raytraceDuration.count()) / 3600;
                        int minutes = (static_cast<int>(_raytraceDuration.count()) % 3600) / 60;
                        int seconds = static_cast<int>(_raytraceDuration.count()) % 60;
                        ImGui::Text("Time elapsed: %02d:%02d:%02d", hours, minutes, seconds);
                    } else {
                        ImGui::Text("Time elapsed: --:--:--");
                    }
                }

                ImGui::EndTable();
            }

            ImGui::EndChild();

            ImGui::BeginChild("Render view", ImVec2(0, 0), true, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            if (_renderId) {
                ImVec2 avail = ImGui::GetContentRegionAvail();

                float aspect = 16.0f / 9.0f;
                float width = avail.x;
                float height = avail.x / aspect;

                if (height > avail.y) {
                    height = avail.y;
                    width = avail.y * aspect;
                }

                float padX = (avail.x - width) * 0.5f;
                float padY = (avail.y - height) * 0.5f;
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + padX);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padY);

                ImGui::Image((ImTextureID)(intptr_t)_renderId, ImVec2(width, height), ImVec2(0, 1), ImVec2(1, 0));
            }
            ImGui::EndChild();
        }

        void renderUI() {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            ImGuizmo::BeginFrame();

            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("Scene")) {
                    if (ImGui::MenuItem("New scene")) {
                        _scene->reset();
                        _scene->getCamera()->setAspect(_viewportSize.x / _viewportSize.y);
                    }

                    const char* filters[] = { "*.gltf", "*.glb" };
                    if (ImGui::MenuItem("Load scene")) {
                        const char* path = tinyfd_openFileDialog("Import scene", "./", 2, filters, NULL, 0);
                        if (path) {
                            _scene->reset();
                            _scene->getCamera()->setAspect(_viewportSize.x / _viewportSize.y);
                            Importer::importFromGLTF(*_scene, path);
                        }
                    }
                    if (ImGui::MenuItem("Save scene")) {
                        const char* path = tinyfd_saveFileDialog("Export scene", "./scene.glb", 2, filters, NULL);
                        if (path) {
                            Exporter::exportToGLTF(_scene->getEntities(), path);
                        }
                    }
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

                        ImGui::Separator();

                        if (ImGui::MenuItem("Import mesh")) {
                            const char* filters[] = { "*.gltf", "*.glb" };
                            const char* path = tinyfd_openFileDialog("Import mesh", "./", 2, filters, NULL, 0);
                            if (path) {
                                MeshImporter::importFromGLTF(*_scene, path);
                            }
                        }

                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Light")) {
                        for (const auto& light : lightList) {
                            if (ImGui::MenuItem(light.name)) {
                                Entity* selected = _scene->getSelectedEntity();
                                if (selected)
                                    selected->setIsSelected(false);

                                Entity* entity = light.create(_scene.get());
                                _scene.get()->setSelectedEntity(entity);
                                entity->setIsSelected(true);
                                entity->setName(_scene->generateUniqueName(light.name));
                            }
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Render")) {
                    if (ImGui::MenuItem("Render scene", nullptr, nullptr, !_raytraceInProgress)) {
                        if (_raytraceThread.joinable())
                            _raytraceThread.join();

                        _raytraceInProgress = true;
                        _raytraceFinished = false;

                        int height = _renderWidth * 9 / 16;
                        _renderData.resize(_renderWidth * height * 3);

                        _scanlineUpdated = false;
                        _scanlineToUpload = -1;

                        std::fill(_renderData.begin(), _renderData.end(), 0);

                        glBindTexture(GL_TEXTURE_2D, _renderId);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _renderWidth, height, 0, GL_RGB, GL_UNSIGNED_BYTE, _renderData.data());
                        glBindTexture(GL_TEXTURE_2D, 0);

                        Raytracer::camera.imageDataBuffer = _renderData.data();
                        Raytracer::camera.onScanlineFinished = [this](int j) {
                            _scanlineUpdated = true;
                        };

                        _raytraceThread = std::thread([this]() {
                            auto start = std::chrono::high_resolution_clock::now();

                            Raytracer::raytrace(_scene->getEntities(), _scene->getSkyboxColor(), _renderWidth, _samplesPerPixel, _maxDepth);
                            
                            auto end = std::chrono::high_resolution_clock::now();
                            _raytraceDuration = end - start;

                            _raytraceInProgress = false;
                            _raytraceFinished = true;
                            _scanlineUpdated = true;
                        });
                    }
                    bool hasRender = !_renderData.empty();
                    if (ImGui::MenuItem("Save current render", nullptr, false, hasRender)) {
                        const char* filters[] = { "*.png" };
                        const char* path = tinyfd_saveFileDialog("Save current render", "./render.png", 1, filters, NULL);
                        if (path) {
                            int height = _renderWidth * 9 / 16;
                            stbi_flip_vertically_on_write(true);
                            stbi_write_png(path, _renderWidth, height, 3, _renderData.data(), _renderWidth * 3);
                        }
                    }
                    if (ImGui::MenuItem("Change render settings")) {
                        _openRenderPopup = true;
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Debug")) {
                    if (ImGui::MenuItem("Set camera transform")) {
                        _openCameraPopup = true;
                    }
                    if (ImGui::MenuItem("Change skybox color")) {
                        _openSkyboxColorPopup = true;
                    }
                    if (ImGui::MenuItem("Save shadow atlases")) {
                        _scene->saveShadowAtlases();
                    }
                    if (ImGui::MenuItem("Save current viewport")) {
                        saveViewport();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
            ImGui::SetNextWindowSize(ImVec2(_width, _height - ImGui::GetFrameHeight()));
            ImGui::Begin("Application", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

            if (ImGui::BeginTabBar("Tabs")) {
                if (ImGui::BeginTabItem("Editor")) {
                    renderEditor();

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Raytracer")) {
                    renderRaytracer();

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::End();

            if (_openSkyboxColorPopup) {
                ImGui::OpenPopup("##Skybox color popup");
            }

            if (ImGui::BeginPopup("##Skybox color popup")) {
                glm::vec3& skyboxColor = _scene->getSkyboxColor();
                float color[3] = { skyboxColor.r, skyboxColor.g, skyboxColor.b };

                ImGui::TextDisabled("Select skybox color");
                ImGui::Separator();

                if (ImGui::ColorPicker3("Skybox color", color)) {
                    skyboxColor = glm::vec3(color[0], color[1], color[2]);
                }

                ImGui::Separator();
                if (ImGui::Button("Close", ImVec2(-1, 0))) {
                    _openSkyboxColorPopup = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            if (_openCameraPopup) {
                ImGui::OpenPopup("##Camera transform popup");
            }

            if (ImGui::BeginPopup("##Camera transform popup")) {
                ImGui::TextDisabled("Camera Transform");
                ImGui::Separator();

                Transform& camTransform = _scene->getCamera()->getTransform();

                ImGui::DragFloat3("Position", glm::value_ptr(camTransform.position), 0.1f);
                if (ImGui::DragFloat2("Rotation", glm::value_ptr(camTransform.rotation), 0.1f))
                    _scene->getCamera()->recalculate();

                ImGui::Separator();
                if (ImGui::Button("Close", ImVec2(-1, 0))) {
                    _openCameraPopup = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            if (_openRenderPopup) {
                ImGui::OpenPopup("##Render popup");
            }

            if (ImGui::BeginPopup("##Render popup")) {
                ImGui::TextDisabled("Render Settings");
                ImGui::Separator();
                
                ImGui::InputInt("Image width", &_renderWidth);
                ImGui::InputInt("Samples per pixel", &_samplesPerPixel);
                ImGui::InputInt("Max depth", &_maxDepth);

                ImGui::Separator();
                if (ImGui::Button("Close", ImVec2(-1, 0))) {
                    _openRenderPopup = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            if (_showAddContextMenu) {
                ImGui::OpenPopup("##ViewportAddMenu");
                _showAddContextMenu = false;
            }

            if (_showDeleteContextMenu) {
                ImGui::OpenPopup("##ViewportDeleteMenu");
                _showDeleteContextMenu = false;
            }

            if (ImGui::BeginPopup("##ViewportAddMenu")) {
                ImGui::TextDisabled("Add");
                ImGui::Separator();

                if (ImGui::BeginMenu("Mesh")) {
                    for (const auto& primitive : primitiveList) {
                        if (ImGui::MenuItem(primitive.name)) {
                            Entity* sel = _scene->getSelectedEntity();
                            if (sel) sel->setIsSelected(false);
                            Entity* entity = primitive.create(_scene.get());
                            _scene->setSelectedEntity(entity);
                            entity->setIsSelected(true);
                            entity->setName(_scene->generateUniqueName(primitive.name));
                            ImGui::CloseCurrentPopup();
                        }
                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem("Import mesh")) {
                        const char* filters[] = { "*.gltf", "*.glb" };
                        const char* path = tinyfd_openFileDialog("Import mesh", "./", 2, filters, NULL, 0);
                        if (path) {
                            MeshImporter::importFromGLTF(*_scene, path);
                        }
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Light")) {
                    for (const auto& light : lightList) {
                        if (ImGui::MenuItem(light.name)) {
                            Entity* sel = _scene->getSelectedEntity();
                            if (sel) sel->setIsSelected(false);
                            Entity* entity = light.create(_scene.get());
                            _scene->setSelectedEntity(entity);
                            entity->setIsSelected(true);
                            entity->setName(_scene->generateUniqueName(light.name));
                            ImGui::CloseCurrentPopup();
                        }
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndPopup();
            }

            if (ImGui::BeginPopup("##ViewportDeleteMenu")) {
                ImGui::TextDisabled("Delete Entity");
                ImGui::Separator();

                Entity* sel = _scene->getSelectedEntity();
                if (sel && sel != _scene->getCamera() && sel != _scene->getGrid()) {
                    ImGui::Text("Delete \"%s\"?", sel->getName().c_str());
                    ImGui::Spacing();
                    if (ImGui::Button("Confirm", ImVec2(120, 0))) {
                        _scene->removeEntity(sel);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                } else {
                    ImGui::TextDisabled("Nothing selected");
                }

                ImGui::EndPopup();
            }

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        static unsigned int loadIcon(const char* path) {
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            int w, h, ch;
            stbi_set_flip_vertically_on_load(false);
            unsigned char* data = stbi_load(path, &w, &h, &ch, 4);

            if (data) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
            }

            return texture;
        }
};

#endif