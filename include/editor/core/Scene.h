#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <memory>
#include <algorithm>
#include <type_traits>
#include <stb_image.h>

#include "Entity.h"
#include "../Camera.h"
#include "../Grid.h"
#include "Light.h"
#include "../mesh/Mesh.h"

class Scene {
    public:
        Scene() {
            initialize();
        }

        void reset() {
            _entities.clear();
            _camera = nullptr;
            _grid = nullptr;
            initialize();
        }

        void update(float deltaTime, const glm::vec2& moveVector, const glm::vec2& lookDelta, bool isRightButtonDown) {
            if (_camera && isRightButtonDown) {
                _camera->handleMove(moveVector, deltaTime);
                _camera->handleLook(lookDelta);
            }

            if (_camera && _grid)
                _grid->handleCameraPos(_camera->getTransform().position);
        }

        void render(float deltaTime) {
            glm::mat4 view = _camera->getViewMatrix();
            glm::mat4 projection = _camera->getProjectionMatrix();

            _meshShader->use();
            _meshShader->setVec3("viewPos", _camera->getTransform().position);

            _gridShader->setViewProjection(view, projection);
            _meshShader->setViewProjection(view, projection);
            _outlineShader->setViewProjection(view, projection);
            _billboardShader->setViewProjection(view, projection);

            sendLightsDataToShader(_meshShader.get());

            for (auto& e : _entities)
                e->render();
        }

        Camera* getCamera() const { return _camera; }
        Grid* getGrid() const { return _grid; }

        const std::vector<std::unique_ptr<Entity>>& getEntities() const {
            return _entities;
        }

        void setSelectedEntity(Entity* e) { _selected = e; }
        Entity* getSelectedEntity() const { return _selected; }

        template<typename T, typename... Args>
        T* createEntity(Args&&... args) {
            auto obj = Entity::Create<T>(std::forward<Args>(args)...);
            T* raw = obj.get();
            if constexpr (std::is_base_of_v<Mesh, T>) {
                raw->setShader(_meshShader.get());
                raw->setSelectedShader(_outlineShader.get());
            } else if constexpr (std::is_base_of_v<Light, T>) {
                raw->setShader(_billboardShader.get());
                raw->setTexture(_lightIcon);
                _lights.push_back(raw);
            }
            _entities.push_back(std::move(obj));
            return raw;
        }

        void removeEntity(Entity* entity) {
            if (!entity) return;

            if (auto light = dynamic_cast<Light*>(entity)) {
                auto it = std::remove(_lights.begin(), _lights.end(), light);
                _lights.erase(it, _lights.end());
            }

            auto it = std::remove_if(_entities.begin(), _entities.end(), 
                [entity](const std::unique_ptr<Entity>& e) { return e.get() == entity; }
            );

            if (it != _entities.end()) {
                _entities.erase(it, _entities.end());
            }

            if (_selected == entity)
                _selected = nullptr;
        }

        std::string generateUniqueName(const std::string& name) {
            std::string base = name;
            if (isNumbered(name))
                base = name.substr(0, name.size() - 4);

            int index = 0;

            for (const auto& e : _entities) {
                std::string n = e->getName();

                if (n.rfind(base, 0) == 0) {
                    if (n.size() == base.size() + 4 && n[base.size()] == '.') {
                        std::string numberStr = n.substr(base.size() + 1);
                        if (numberStr.size() == 3 && std::isdigit(numberStr[0]) && std::isdigit(numberStr[1]) && std::isdigit(numberStr[2])) {
                            int number = std::stoi(numberStr);
                            if (number > index)
                                index = number;
                        }
                    }
                }
            }

            char unique[64];
            std::snprintf(unique, sizeof(unique), "%s.%03d", base.c_str(), index + 1);
            return std::string(unique);
        }

        bool isNameTaken(const std::string& name) const {
            for (const auto& e : _entities) {
                if (e->getName() == name)
                    return true;
            }
            return false;
        }

        bool isNumbered(const std::string& name) const {
            if (name.size() < 4) return false;
            if (name[name.size() - 4] != '.') return false;

            return std::isdigit(name[name.size() - 3]) && std::isdigit(name[name.size() - 2]) && std::isdigit(name[name.size() - 1]);
        }
    private:
        std::vector<std::unique_ptr<Entity>> _entities;
        std::vector<Light*> _lights;
        Entity* _selected;
        Camera* _camera;
        Grid* _grid;

        std::unique_ptr<Shader> _gridShader;
        std::unique_ptr<Shader> _meshShader;
        std::unique_ptr<Shader> _outlineShader;
        std::unique_ptr<Shader> _billboardShader;

        unsigned int _lightIcon;

        unsigned int loadTexture(std::string path) {
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_set_flip_vertically_on_load(true);

            int width, height, nrChannels;
            unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
            if (data) {
                GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
                glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);
            } else {
                std::cout << "Failed to load texture" << std::endl;
            }
            stbi_image_free(data);
            return texture;
        }

        void initialize() {
            _gridShader = std::make_unique<Shader>("assets/shaders/grid.vs", "assets/shaders/grid.fs");
            _meshShader = std::make_unique<Shader>("assets/shaders/default.vs", "assets/shaders/lit.fs");
            _outlineShader = std::make_unique<Shader>("assets/shaders/default.vs", "assets/shaders/unlit.fs");
            _billboardShader = std::make_unique<Shader>("assets/shaders/billboard.vs", "assets/shaders/billboard.fs");

            _meshShader->use();

            _meshShader->setVec3("light.direction", {-0.2f, -1.0f, -0.3f});
            _meshShader->setVec3("light.ambient", {0.2f, 0.2f, 0.2f});
            _meshShader->setVec3("light.diffuse", {0.5f, 0.5f, 0.5f});
            _meshShader->setVec3("light.specular", {1.0f, 1.0f, 1.0f});

            _meshShader->setVec3("material.ambient", {1.0f, 0.5f, 0.31f});
            _meshShader->setVec3("material.diffuse", {1.0f, 0.5f, 0.31f});
            _meshShader->setVec3("material.specular", {0.5f, 0.5f, 0.5f});
            _meshShader->setFloat("material.shininess", 32.0f);

            _outlineShader->use();
            _outlineShader->setVec3("color", {1.0f, 1.0f, 1.0f});

            _billboardShader->use();
            _billboardShader->setInt("tex", 0);

            _camera = createEntity<Camera>();
            _grid = createEntity<Grid>();   
            _grid->setShader(_gridShader.get());

            _lightIcon = loadTexture("assets/textures/lightbulb.png");
        }

        void sendLightsDataToShader(Shader* shader) {

        }
};

#endif