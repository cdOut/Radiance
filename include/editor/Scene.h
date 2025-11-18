#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <memory>
#include <algorithm>
#include <type_traits>
#include <stb_image.h>

#include "entity/Entity.h"
#include "entity/util/Camera.h"
#include "entity/util/Grid.h"
#include "entity/light/Light.h"
#include "entity/mesh/Mesh.h"

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
            glEnable(GL_STENCIL_TEST);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); 
            glClear(GL_STENCIL_BUFFER_BIT);

            glStencilMask(0x00);

            glm::mat4 view = _camera->getViewMatrix();
            glm::mat4 projection = _camera->getProjectionMatrix();

            _meshShader->use();
            _meshShader->setVec3("viewPos", _camera->getTransform().position);
            uploadLightsToShader(_meshShader.get());

            _gridShader->setViewProjection(view, projection);
            _meshShader->setViewProjection(view, projection);
            _outlineShader->setViewProjection(view, projection);
            _billboardShader->setViewProjection(view, projection);

            for (const auto& [_, e] : _entities) {
                if (!dynamic_cast<Light*>(e.get()))
                    e->render();
            }

            for (Light* light : _lights) {
                light->render();
            }

            if (Mesh* mesh = dynamic_cast<Mesh*>(_selected)) {
                glm::mat4 model = mesh->getModelMatrix();
                glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                Material& material = mesh->getMaterial();

                glStencilMask(0xFF);
                glStencilFunc(GL_ALWAYS, 1, 0xFF);  
                glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

                glDisable(GL_DEPTH_TEST);
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

                _meshShader->use();
                _meshShader->setMat4("model", model);
                _meshShader->setMat3("normalMatrix", normalMatrix);
                _meshShader->setVec3("albedo", glm::pow(material.albedo, glm::vec3(2.2f)));
                _meshShader->setFloat("metallic", material.metallic);
                _meshShader->setFloat("roughness", material.roughness);

                mesh->renderGeometry();

                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                glEnable(GL_DEPTH_TEST);

                glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
                glStencilMask(0x00);
                glDisable(GL_DEPTH_TEST);

                glm::mat4 outlineModel = glm::scale(model, glm::vec3(1.05f));

                _outlineShader->use();
                _outlineShader->setMat4("model", outlineModel);
                _outlineShader->setMat3("normalMatrix", normalMatrix);
                _outlineShader->setVec3("color", glm::vec3(1.0f));

                mesh->renderGeometry();

                glEnable(GL_DEPTH_TEST);
            }

            glStencilMask(0xFF);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glDisable(GL_STENCIL_TEST);
        }

        void renderGpuSelect() {
            glm::mat4 view = _camera->getViewMatrix();
            glm::mat4 projection = _camera->getProjectionMatrix();

            _gpuSelectShader->setViewProjection(view, projection);
            Shader* shader;

            for (const auto& [_, e] : _entities) {
                if (!dynamic_cast<Light*>(e.get())) {
                    if (e.get() == _camera || e.get() == _grid)
                        continue;

                    _gpuSelectShader->setVec3("idColor", e->getIdColor());
                    _gpuSelectShader->setInt("isBillboard", false);
                    
                    shader = e->getShader();
                    e->setShader(_gpuSelectShader.get());
                    e->render();
                    e->setShader(shader);
                }
            }

            for (Light* light : _lights) {
                _gpuSelectShader->setVec3("idColor", light->getIdColor());
                _gpuSelectShader->setInt("isBillboard", true);
                
                shader = light->getShader();
                light->setShader(_gpuSelectShader.get());
                light->render();
                light->setShader(shader);
            }
        }

        Camera* getCamera() const { return _camera; }
        Grid* getGrid() const { return _grid; }

        const std::unordered_map<int, std::unique_ptr<Entity>>& getEntities() const {
            return _entities;
        }

        void setSelectedEntity(Entity* e) { _selected = e; }
        Entity* getSelectedEntity() const { return _selected; }

        template<typename T, typename... Args>
        T* createEntity(Args&&... args) {
            auto obj = Entity::Create<T>(std::forward<Args>(args)...);
            T* raw = obj.get();

            int id = idVar++;
            raw->setId(id);
            raw->setSelectedShader(_outlineShader.get());

            if constexpr (std::is_base_of_v<Mesh, T>) {
                raw->setShader(_meshShader.get());
            } else if constexpr (std::is_base_of_v<Light, T>) {
                raw->setShader(_billboardShader.get());
                raw->setTextures(_lightIcon, _lightIconSelected);
                _lights.push_back(raw);
            }

            _entities[id] = std::move(obj);
            return raw;
        }

        void removeEntity(Entity* entity) {
            if (!entity) return;

            if (auto light = dynamic_cast<Light*>(entity)) {
                auto it = std::remove(_lights.begin(), _lights.end(), light);
                _lights.erase(it, _lights.end());
            }

            int id = entity->getId();
            _entities.erase(id);

            if (_selected == entity)
                _selected = nullptr;
        }

        std::string generateUniqueName(const std::string& name) {
            std::string base = name;
            if (isNumbered(name))
                base = name.substr(0, name.size() - 4);

            int index = 0;

            for (const auto& [_, e] : _entities) {
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
            for (const auto& [_, e] : _entities) {
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
        std::unordered_map<int, std::unique_ptr<Entity>> _entities;
        std::vector<Light*> _lights;
        Entity* _selected;
        Camera* _camera;
        Grid* _grid;

        unsigned int idVar = 0;

        std::unique_ptr<Shader> _gridShader;
        std::unique_ptr<Shader> _meshShader;
        std::unique_ptr<Shader> _outlineShader;
        std::unique_ptr<Shader> _billboardShader;
        std::unique_ptr<Shader> _gpuSelectShader;

        unsigned int _lightIcon, _lightIconSelected;

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
            _gpuSelectShader = std::make_unique<Shader>("assets/shaders/gpuSelect.vs", "assets/shaders/gpuSelect.fs");

            _meshShader->use();

            _outlineShader->use();
            _outlineShader->setVec3("color", {1.0f, 1.0f, 1.0f});

            _billboardShader->use();
            _billboardShader->setInt("tex", 0);

            _camera = createEntity<Camera>();
            _grid = createEntity<Grid>();   
            _grid->setShader(_gridShader.get());

            _lightIcon = loadTexture("assets/textures/lightbulbEmpty.png");
            _lightIconSelected = loadTexture("assets/textures/lightbulb.png");
        }

        void uploadLightsToShader(Shader* shader) {
            unsigned int dirIndex = 0, pointIndex = 0, spotIndex = 0;
            
            for (Light* light : _lights) {
                switch (light->getType()) {
                    case LightType::Directional:
                        light->uploadToShader(shader, dirIndex++);
                        break;
                    case LightType::Point:
                        light->uploadToShader(shader, pointIndex++);
                        break;
                    case LightType::Spot:
                        light->uploadToShader(shader, spotIndex++);
                        break;
                    default:
                        break;
                }
            }

            shader->setInt("directionalLightsAmount", dirIndex);
            shader->setInt("pointLightsAmount", pointIndex);
            shader->setInt("spotLightsAmount", spotIndex);
        }
};

#endif