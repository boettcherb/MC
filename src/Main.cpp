#include "Constants.h"
#include "Shader.h"
#include "Camera.h"
#include "Texture.h"
#include "ChunkLoader.h"
#include <glad/glad.h>
#include <GLFW/GLFW3.h>
#include <iostream>
#include <string>

// from UI.cpp
void initialize_HUD(int width, int height);
void resize_HUD(int width, int height);
void render_HUD(Shader* shader);

static unsigned int g_screenWidth = 1200;
static unsigned int g_screenHeight = 800;
static Camera camera({ 16.5f, 80.0f, 16.5f });
static bool g_mouse_captured = true;

// This callback function executes whenever the user changes the window size
void window_size_callback(GLFWwindow* /* window */, int width, int height) {
    g_screenWidth = width;
    g_screenHeight = height;
    glViewport(0, 0, width, height);
    resize_HUD(width, height);
}

// This callback function executes whenever the user moves the mouse
void mouse_callback(GLFWwindow* /* window */, double xpos, double ypos) {
    if (g_mouse_captured) {
        camera.processMouseMovement((float) xpos, (float) ypos);
    }
}

// This callback function executes whenever the user moves the mouse scroll wheel
void scroll_callback(GLFWwindow* /* window */, double /* offsetX */, double offsetY) {
    if (g_mouse_captured) {
        camera.processMouseScroll((float) offsetY);
    }
}

// The callback function executes whenever a key is pressed or released
void key_callback(GLFWwindow* window, int key, int /* scancode */, int action, int mods) {
    // If the escape key is pressed, close the window. If the escape key is
    // pressed while shift is pressed, toggle holding / releasing the mouse.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (mods & GLFW_MOD_SHIFT) {
            if (g_mouse_captured) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                g_mouse_captured = false;
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                g_mouse_captured = true;
            }
        } else {
            glfwSetWindowShouldClose(window, true);
        }
    }
}

// Called every frame inside the render loop
static void processInput(GLFWwindow* window, float deltaTime) {
    // WASD for the camera
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.processKeyboard(Movement::FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.processKeyboard(Movement::BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.processKeyboard(Movement::LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.processKeyboard(Movement::RIGHT, deltaTime);
    }
}

// print the FPS to the screen every second
static void displayFPS() {
    static int FPS = 0;
    static double previousTime = glfwGetTime();
    double currentTime = glfwGetTime();
    ++FPS;
    if (currentTime - previousTime >= 1.0) {
        std::cout << "FPS: " << FPS << '\n';
        FPS = 0;
        previousTime = currentTime;
    }
}

int main() {
    // initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_SAMPLES, 1); // anti-aliasing is causing lines between blocks

    // create the main window
    GLFWwindow* window = glfwCreateWindow(g_screenWidth, g_screenHeight,
                                          WINDOW_TITLE, nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwTerminate();
        return -1;
    }

    initialize_HUD(g_screenWidth, g_screenHeight);

    /////////////////////////////////////////////////////////////////////////////////



    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << '\n';
    std::cout << "Starting Application...\n";

    // tell GLFW to capture our mouse cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // enable VSync (tie the FPS to your monitor's refresh rate)
    glfwSwapInterval(1);

    Shader blockShader(BLOCK_VERTEX, BLOCK_FRAGMENT);
    Shader uiShader(UI_VERTEX, UI_FRAGMENT);
    Texture textureSheet(TEXTURE_SHEET, 0);
    blockShader.addTexture(&textureSheet, "u3_texture");
    uiShader.addTexture(&textureSheet, "u3_texture");
    
    int camX = (int) camera.getPosition().x / CHUNK_LENGTH - (camera.getPosition().x < 0);
    int camZ = (int) camera.getPosition().z / CHUNK_WIDTH - (camera.getPosition().z < 0);
    ChunkLoader chunkLoader(&blockShader, camX, camZ);

    glClearColor(0.2f, 0.3f, 0.8f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // variables for deltaTime
    double previousTime = glfwGetTime();
    double deltaTime = 0.0f;

    // render loop
    while (!glfwWindowShouldClose(window)) {
        displayFPS();
        double currentTime = glfwGetTime();
        deltaTime = currentTime - previousTime;
        previousTime = currentTime;
        if (g_mouse_captured) {
            processInput(window, (float) deltaTime);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        chunkLoader.update(&camera);
        chunkLoader.renderAll(camera, (float) g_screenWidth / g_screenHeight);
        render_HUD(&uiShader);


#ifndef NDEBUG
        // catch errors
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cout << "OpenGL Error (in main): " << err << '\n';
        }
#endif

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
