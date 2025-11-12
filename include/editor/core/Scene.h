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

            for (auto& e : _entities)
                e->render();
        }

        Camera* getCamera() const { return _camera; }

        template<typename T, typename... Args>
        T* createEntity(Args&&... args) {
            auto obj = Entity::Create<T>(std::forward<Args>(args)...);
            T* raw = obj.get();
            if (std::is_base_of<Mesh, T>::value) {
                raw->setShader(_meshShader.get());
            }
            _entities.push_back(std::move(obj));
            return raw;
        }
    private:
        std::vector<std::unique_ptr<Entity>> _entities;
        Camera* _camera;
        Grid* _grid;

        std::unique_ptr<Shader> _gridShader;
        std::unique_ptr<Shader> _meshShader;

        void initialize() {
            _gridShader = std::make_unique<Shader>("assets/shaders/grid.vs", "assets/shaders/grid.fs");
            _meshShader = std::make_unique<Shader>("assets/shaders/baseShader.vs", "assets/shaders/litShader.fs");

            _meshShader->use();
            _meshShader->setVec3("color", {1.0f, 0.5f, 0.31f});
            _meshShader->setVec3("lightColor", {1.0f, 1.0f, 1.0f});
            _meshShader->setVec3("lightPos", {0.0f, 1.0f, 2.0f});
            _meshShader->setVec3("viewPos", {0.0f, 0.0f, 3.0f});

            _camera = createEntity<Camera>();
            _grid = createEntity<Grid>();   
            _grid->setShader(_gridShader.get());
        }
};

#endif