#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include "shader.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <filesystem>
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// ── State ─────────────────────────────────────────────────────────────────────

bool cursorEnabled = false;  // false = captured (game mode), true = free (UI mode)
bool tabHeld = false;
bool enableSSR = true;
bool spaceHeld = false;
int  debugMode = 0;    // 0=SSR, 1=pos, 2=normal, 3=albedo
bool key1Held = false, key2Held = false,
key3Held = false, key4Held = false;

// ── Camera ────────────────────────────────────────────────────────────────────
glm::vec3 cameraPos = glm::vec3(0.0f, 2.5f, 7.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.3f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f, pitch = -15.0f;
float lastX = SCR_WIDTH / 2.0f, lastY = SCR_HEIGHT / 2.0f;
bool  firstMouse = true;
float deltaTime = 0.0f, lastFrame = 0.0f;

// ── FPS tracking ──────────────────────────────────────────────────────────────
float fpsTimer = 0.0f;
int   frameCount = 0;
float currentFPS = 0.0f;
float frameMs = 0.0f;

// ── G-Buffer ──────────────────────────────────────────────────────────────────
unsigned int gBuffer, gPosition, gNormal, gAlbedoSpec, rboDepth;
unsigned int quadVAO = 0, quadVBO;

// ── Callbacks ─────────────────────────────────────────────────────────────────
void framebuffer_size_callback(GLFWwindow* w, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* w, double xposIn, double yposIn) {

    if (cursorEnabled) return;
    float xpos = (float)xposIn, ypos = (float)yposIn;
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = (xpos - lastX) * 0.1f;
    float yoffset = (lastY - ypos) * 0.1f;
    lastX = xpos; lastY = ypos;
    yaw += xoffset; pitch += yoffset;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void processInput(GLFWwindow* w) {
    float speed = 3.0f * deltaTime;
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(w, true);
    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) cameraPos += speed * cameraFront;
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= speed * cameraFront;
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;

    // SPACE — toggle SSR
    if (glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS && !spaceHeld) {
        enableSSR = !enableSSR;
        spaceHeld = true;
        std::cout << "[SSR] " << (enableSSR ? "ON" : "OFF")
            << " | FPS: " << currentFPS
            << " | Frame: " << frameMs << " ms" << std::endl;
    }
    if (glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_RELEASE) spaceHeld = false;

    // B — print parameter benchmark to console
    static bool bHeld = false;
    if (glfwGetKey(w, GLFW_KEY_B) == GLFW_PRESS && !bHeld) {
        bHeld = true;
        std::cout << "\n===== SSR Parameter Analysis (McGuire & Mara 2014) =====" << std::endl;
        std::cout << "Current MAX_STEPS : 64  | STEP_SIZE: 0.2 | BINARY_STEPS: 8" << std::endl;
        std::cout << "Current FPS       : " << (int)currentFPS << std::endl;
        std::cout << "Current Frame Time: " << frameMs << " ms" << std::endl;
        std::cout << "SSR Status        : " << (enableSSR ? "ON" : "OFF") << std::endl;
        std::cout << "Debug Mode        : " << debugMode << std::endl;
        std::cout << "--------------------------------------------------------" << std::endl;
        std::cout << "Trade-off: Higher MAX_STEPS = better quality, lower FPS" << std::endl;
        std::cout << "Binary search refinement reduces position error after hit" << std::endl;
        std::cout << "========================================================\n" << std::endl;
    }
    if (glfwGetKey(w, GLFW_KEY_B) == GLFW_RELEASE) bHeld = false;

    // TAB — toggle mouse cursor for ImGui interaction
    if (glfwGetKey(w, GLFW_KEY_TAB) == GLFW_PRESS && !tabHeld) {
        tabHeld = true;
        cursorEnabled = !cursorEnabled;
        if (cursorEnabled) {
            glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            firstMouse = true;  // prevent camera jump when re-capturing
        }
        else {
            glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if (glfwGetKey(w, GLFW_KEY_TAB) == GLFW_RELEASE) tabHeld = false;

    // 1/2/3/4 — G-Buffer debug modes
    if (glfwGetKey(w, GLFW_KEY_1) == GLFW_PRESS && !key1Held) { debugMode = 1; key1Held = true; std::cout << "[DEBUG] Position buffer\n"; }
    if (glfwGetKey(w, GLFW_KEY_1) == GLFW_RELEASE) key1Held = false;
    if (glfwGetKey(w, GLFW_KEY_2) == GLFW_PRESS && !key2Held) { debugMode = 2; key2Held = true; std::cout << "[DEBUG] Normal buffer\n"; }
    if (glfwGetKey(w, GLFW_KEY_2) == GLFW_RELEASE) key2Held = false;
    if (glfwGetKey(w, GLFW_KEY_3) == GLFW_PRESS && !key3Held) { debugMode = 3; key3Held = true; std::cout << "[DEBUG] Albedo buffer\n"; }
    if (glfwGetKey(w, GLFW_KEY_3) == GLFW_RELEASE) key3Held = false;
    if (glfwGetKey(w, GLFW_KEY_4) == GLFW_PRESS && !key4Held) { debugMode = 0; key4Held = true; std::cout << "[DEBUG] SSR mode\n"; }
    if (glfwGetKey(w, GLFW_KEY_4) == GLFW_RELEASE) key4Held = false;
}

// ── G-Buffer setup ────────────────────────────────────────────────────────────
void setupGBuffer() {
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);

    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR: G-Buffer incomplete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ── Fullscreen quad ───────────────────────────────────────────────────────────
void renderQuad() {
    if (quadVAO == 0) {
        float quadVertices[] = {
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// ── Cube VAO ──────────────────────────────────────────────────────────────────
unsigned int setupCube() {
    float v[] = {
        -0.5f,-0.5f,-0.5f, 0.0f, 0.0f,-1.0f,
         0.5f,-0.5f,-0.5f, 0.0f, 0.0f,-1.0f,
         0.5f, 0.5f,-0.5f, 0.0f, 0.0f,-1.0f,
         0.5f, 0.5f,-0.5f, 0.0f, 0.0f,-1.0f,
        -0.5f, 0.5f,-0.5f, 0.0f, 0.0f,-1.0f,
        -0.5f,-0.5f,-0.5f, 0.0f, 0.0f,-1.0f,
        -0.5f,-0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
         0.5f,-0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
         0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
         0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f,-0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, 0.5f,-1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f,-0.5f,-1.0f, 0.0f, 0.0f,
        -0.5f,-0.5f,-0.5f,-1.0f, 0.0f, 0.0f,
        -0.5f,-0.5f,-0.5f,-1.0f, 0.0f, 0.0f,
        -0.5f,-0.5f, 0.5f,-1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.5f,-1.0f, 0.0f, 0.0f,
         0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
         0.5f, 0.5f,-0.5f, 1.0f, 0.0f, 0.0f,
         0.5f,-0.5f,-0.5f, 1.0f, 0.0f, 0.0f,
         0.5f,-0.5f,-0.5f, 1.0f, 0.0f, 0.0f,
         0.5f,-0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
         0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
        -0.5f,-0.5f,-0.5f, 0.0f,-1.0f, 0.0f,
         0.5f,-0.5f,-0.5f, 0.0f,-1.0f, 0.0f,
         0.5f,-0.5f, 0.5f, 0.0f,-1.0f, 0.0f,
         0.5f,-0.5f, 0.5f, 0.0f,-1.0f, 0.0f,
        -0.5f,-0.5f, 0.5f, 0.0f,-1.0f, 0.0f,
        -0.5f,-0.5f,-0.5f, 0.0f,-1.0f, 0.0f,
        -0.5f, 0.5f,-0.5f, 0.0f, 1.0f, 0.0f,
         0.5f, 0.5f,-0.5f, 0.0f, 1.0f, 0.0f,
         0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
         0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f,-0.5f, 0.0f, 1.0f, 0.0f,
    };
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
    return VAO;
}

// ── Plane VAO ─────────────────────────────────────────────────────────────────
unsigned int setupPlane() {
    float v[] = {
        -6.0f, 0.0f, -6.0f,  0.0f, 1.0f, 0.0f,
         6.0f, 0.0f, -6.0f,  0.0f, 1.0f, 0.0f,
         6.0f, 0.0f,  6.0f,  0.0f, 1.0f, 0.0f,
         6.0f, 0.0f,  6.0f,  0.0f, 1.0f, 0.0f,
        -6.0f, 0.0f,  6.0f,  0.0f, 1.0f, 0.0f,
        -6.0f, 0.0f, -6.0f,  0.0f, 1.0f, 0.0f,
    };
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
    return VAO;
}

// ── Main ──────────────────────────────────────────────────────────────────────
int main() {

    std::filesystem::current_path(
        std::filesystem::path(__FILE__).parent_path().parent_path()
    );
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
        "SSR | SPACE=toggle SSR | TAB=cursor | 1-4=G-Buffer | ESC=quit", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);

    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    Shader gbufferShader("shaders/gbuffer.vert", "shaders/gbuffer.frag");
    Shader ssrtShader("shaders/ssrt.vert", "shaders/ssrt.frag");

    unsigned int cubeVAO = setupCube();
    unsigned int planeVAO = setupPlane();
    setupGBuffer();

    glm::mat4 projection = glm::perspective(glm::radians(45.0f),
        (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

    ssrtShader.use();
    ssrtShader.setInt("gPosition", 0);
    ssrtShader.setInt("gNormal", 1);
    ssrtShader.setInt("gAlbedoSpec", 2);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        frameMs = deltaTime * 1000.0f;

        // FPS counter — update every second
        fpsTimer += deltaTime;
        frameCount++;
        if (fpsTimer >= 1.0f) {
            currentFPS = (float)frameCount / fpsTimer;
            std::cout << "FPS: " << (int)currentFPS
                << " | Frame: " << frameMs << " ms"
                << " | SSR: " << (enableSSR ? "ON" : "OFF")
                << " | Mode: " << debugMode << std::endl;
            fpsTimer = 0.0f;
            frameCount = 0;
        }

        processInput(window);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        // ── Pass 1: Geometry → G-Buffer ───────────────────────────────────────
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gbufferShader.use();
        gbufferShader.setMat4("projection", projection);
        gbufferShader.setMat4("view", view);

        // Floor — highly reflective grey
        glm::mat4 model = glm::mat4(1.0f);
        gbufferShader.setMat4("model", model);
        gbufferShader.setVec3("albedo", glm::vec3(0.7f, 0.7f, 0.75f));
        gbufferShader.setFloat("reflectivity", 0.9f);
        glBindVertexArray(planeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Cube 1 — red, matte
        model = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.5f, -1.0f));
        gbufferShader.setMat4("model", model);
        gbufferShader.setVec3("albedo", glm::vec3(0.9f, 0.2f, 0.2f));
        gbufferShader.setFloat("reflectivity", 0.05f);
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Cube 2 — green, semi reflective
        model = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.5f, 0.5f));
        gbufferShader.setMat4("model", model);
        gbufferShader.setVec3("albedo", glm::vec3(0.2f, 0.8f, 0.3f));
        gbufferShader.setFloat("reflectivity", 0.3f);
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Cube 3 — blue, tall, matte
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, -2.5f));
        model = glm::scale(model, glm::vec3(1.0f, 2.0f, 1.0f));
        gbufferShader.setMat4("model", model);
        gbufferShader.setVec3("albedo", glm::vec3(0.2f, 0.3f, 0.95f));
        gbufferShader.setFloat("reflectivity", 0.05f);
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Cube 4 — yellow MOVING
        float angle = (float)glfwGetTime() * 0.8f;
        float rx = sin(angle) * 3.5f;
        float rz = cos(angle) * 3.5f - 1.0f;
        model = glm::translate(glm::mat4(1.0f), glm::vec3(rx, 0.5f, rz));
        model = glm::rotate(model, angle * 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        gbufferShader.setMat4("model", model);
        gbufferShader.setVec3("albedo", glm::vec3(1.0f, 0.85f, 0.1f));
        gbufferShader.setFloat("reflectivity", 0.05f);
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Back wall — semi reflective, shows reflections of scene
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, -5.5f));
        model = glm::scale(model, glm::vec3(12.0f, 4.0f, 0.2f));
        gbufferShader.setMat4("model", model);
        gbufferShader.setVec3("albedo", glm::vec3(0.85f, 0.82f, 0.78f));
        gbufferShader.setFloat("reflectivity", 0.4f);
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // ── Pass 2: SSR ───────────────────────────────────────────────────────
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ssrtShader.use();
        ssrtShader.setMat4("projection", projection);
        ssrtShader.setBool("enableSSR", enableSSR);
        ssrtShader.setInt("debugMode", debugMode);

        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);

        renderQuad();

        // ── ImGui UI overlay ──────────────────────────────────────────────────────
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(320, 220), ImGuiCond_Always);
        ImGui::Begin("SSR Debug Panel", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        // Performance
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Performance");
        ImGui::Text("FPS        : %.1f", currentFPS);
        ImGui::Text("Frame Time : %.2f ms", frameMs);
        ImGui::Separator();

        // SSR toggle
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Controls");
        if (ImGui::Checkbox("Enable SSR  [SPACE]", &enableSSR)) {}
        ImGui::Separator();

        // G-Buffer mode
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "G-Buffer View");
        ImGui::RadioButton("Full SSR  [4]", &debugMode, 0); ImGui::SameLine();
        ImGui::RadioButton("Position [1]", &debugMode, 1);
        ImGui::RadioButton("Normals  [2]", &debugMode, 2); ImGui::SameLine();
        ImGui::RadioButton("Albedo   [3]", &debugMode, 3);
        ImGui::Separator();

        // Paper info
        ImGui::TextColored(ImVec4(0.5f, 1, 0.5f, 1), "McGuire & Mara 2014");
        ImGui::Text("MAX_STEPS    : 64");
        ImGui::Text("BINARY_STEPS : 8");
        ImGui::Text("THICKNESS    : 0.5");

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