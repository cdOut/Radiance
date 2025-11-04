#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include "Shader.h"
#include "mesh/Cone.h"
#include "mesh/Cylinder.h"
#include "mesh/Sphere.h"
#include "mesh/Cube.h"
#include "mesh/Plane.h"
#include "mesh/Torus.h"
#include "Camera.h"

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow *window, glm::vec2& moveVector, glm::vec2& lookDeltaVector);

float lastX = 400, lastY = 300;
bool firstMouse = true;

glm::vec2 moveVector;
glm::vec2 lookDeltaVector;

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    GLFWwindow *window = glfwCreateWindow(800, 600, "raytracing-thesis", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    int windowWidth, windowHeight;
    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);  

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    glEnable(GL_DEPTH_TEST);

    Shader litShader("assets/shaders/baseShader.vs", "assets/shaders/litShader.fs");

    Camera camera(90.0f, 800.0f/600.0f, 0.1f, 100.0f);
    
    auto mesh = Mesh::Create<Sphere>();

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame; 

        processInput(window, moveVector, lookDeltaVector);

        camera.handleMove(moveVector, deltaTime);
        camera.handleLook(lookDeltaVector);
        lookDeltaVector = {0.0f, 0.0f};

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        litShader.use();
        litShader.setMat4("view", camera.getViewMatrix());
        litShader.setMat4("projection", camera.getProjectionMatrix());

        litShader.setVec3("color", {1.0f, 0.5f, 0.31f});
        litShader.setVec3("lightColor", {1.0f, 1.0f, 1.0f});
        litShader.setVec3("lightPos", {0.0f, 1.0f, 2.0f});
        litShader.setVec3("viewPos", {0.0f, 0.0f, 3.0f});

        mesh->render(litShader);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Mesh Controls");

        Transform& t = mesh->getTransform();
        glm::vec3& c = mesh->getColor();

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Position", glm::value_ptr(t.position), 0.1f);
            ImGui::DragFloat3("Rotation", glm::value_ptr(t.rotation), 1.0f);
            ImGui::DragFloat3("Scale", glm::value_ptr(t.scale), 0.05f, 0.01f, 10.0f);
        }

        if (ImGui::CollapsingHeader("Color", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::ColorEdit3("Color", glm::value_ptr(c));
        }

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    lookDeltaVector = {xoffset, yoffset};
}

void processInput(GLFWwindow *window, glm::vec2& moveVector, glm::vec2& lookDeltaVector) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    moveVector = {0.0f, 0.0f};

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveVector.y += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveVector.y -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveVector.x -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveVector.x += 1.0f;

    if (glm::length(moveVector) > 1.0f)
        glm::normalize(moveVector);

    
}