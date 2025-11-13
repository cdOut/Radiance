#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <memory>
#include "Entity.h"
#include "../Camera.h"
#include "../Grid.h"
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

            _gridShader->setViewProjection(view, projection);
            _meshShader->setViewProjection(view, projection);
            _outlineShader->setViewProjection(view, projection);

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
            if (std::is_base_of<Mesh, T>::value) {
                raw->setShader(_meshShader.get());
                raw->setSelectedShader(_outlineShader.get());
            }
            _entities.push_back(std::move(obj));
            return raw;
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
        Entity* _selected;
        Camera* _camera;
        Grid* _grid;

        std::unique_ptr<Shader> _gridShader;
        std::unique_ptr<Shader> _meshShader;
        std::unique_ptr<Shader> _outlineShader;

        void initialize() {
            _gridShader = std::make_unique<Shader>("assets/shaders/grid.vs", "assets/shaders/grid.fs");
            _meshShader = std::make_unique<Shader>("assets/shaders/default.vs", "assets/shaders/lit.fs");
            _outlineShader = std::make_unique<Shader>("assets/shaders/default.vs", "assets/shaders/unlit.fs");

            _meshShader->use();
            _meshShader->setVec3("color", {1.0f, 0.5f, 0.31f});
            _meshShader->setVec3("lightColor", {1.0f, 1.0f, 1.0f});
            _meshShader->setVec3("lightPos", {0.0f, 1.0f, 2.0f});
            _meshShader->setVec3("viewPos", {0.0f, 0.0f, 3.0f});

            _outlineShader->use();
            _outlineShader->setVec3("color", {1.0f, 1.0f, 1.0f});

            _camera = createEntity<Camera>();
            _grid = createEntity<Grid>();   
            _grid->setShader(_gridShader.get());
        }
};

#endif