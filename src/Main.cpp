#include "ShaderProgram.h"
#include "Camera.h"
#include "Texture.h"
#include "BlockInfo.h"
#include "Chunk.h"

#include <glad/glad.h>
#include <GLFW/GLFW3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <new>

static unsigned int g_scrWidth = 800;
static unsigned int g_scrHeight = 600;
const char* WINDOW_TITLE = "OpenGL Window";

// This callback function executes whenever the user moves the mouse
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    Camera* camera = reinterpret_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (camera) {
        camera->processMouseMovement(static_cast<float>(xpos), static_cast<float>(ypos));
    }
}

// This callback function executes whenever the user moves the mouse scroll wheel
void scroll_callback(GLFWwindow* window, double /* offsetX */, double offsetY) {
    Camera* camera = reinterpret_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (camera) {
        camera->processMouseScroll(static_cast<float>(offsetY));
    }
}

// Called every frame inside the render loop
static void processInput(GLFWwindow* window, Camera* camera, float deltaTime) {
    // if the escape key is pressed, tell the window to close
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // WASD for the camera
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera->processKeyboard(Camera::FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera->processKeyboard(Camera::BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera->processKeyboard(Camera::LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera->processKeyboard(Camera::RIGHT, deltaTime);
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // create the main window
    GLFWwindow* window = glfwCreateWindow(g_scrWidth, g_scrHeight, WINDOW_TITLE, nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwTerminate();
        return -1;
    }


    /////////////////////////////////////////////////////////////////////////////////


    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << '\n';
    std::cout << "Starting Application...\n";

    // tell GLFW to capture our mouse cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // enable VSync (tie the FPS to your monitor's refresh rate)
    glfwSwapInterval(1);

    // Set the camera object as the window's user pointer. This makes it accessible 
    // in callback functions by using glfwGetWindowUserPointer().
    Camera camera(glm::vec3(0.0f, 80.0f, 0.0f));
    glfwSetWindowUserPointer(window, reinterpret_cast<void*>(&camera));

    ShaderProgram shader("res/shaders/basic_vertex.glsl", "res/shaders/basic_fragment.glsl");
    Texture textureSheet("res/textures/texture_sheet.png", 0);
    shader.addTexture(&textureSheet, "u_texture");

    const int numChunksX = 20;
    const int numChunksZ = 20;
    Chunk* chunks[numChunksX][numChunksZ];
    for (int x = 0; x < numChunksX; ++x) {
        for (int z = 0; z < numChunksZ; ++z) {
           chunks[x][z] = new Chunk(static_cast<float>(x), static_cast<float>(z), &shader);
        }
    }
    for (int x = 0; x < numChunksX; ++x) {
        for (int z = 0; z < numChunksZ; ++z) {
            if (x > 0) chunks[x][z]->addNeighbor(chunks[x - 1][z], Chunk::MINUS_X);
            if (z > 0) chunks[x][z]->addNeighbor(chunks[x][z - 1], Chunk::MINUS_Z);
            if (x < numChunksX - 1) chunks[x][z]->addNeighbor(chunks[x + 1][z], Chunk::PLUS_X);
            if (z < numChunksZ - 1) chunks[x][z]->addNeighbor(chunks[x][z + 1], Chunk::PLUS_Z);
        }
    }
    for (int x = 0; x < numChunksX; ++x) {
        for (int z = 0; z < numChunksZ; ++z) {
            chunks[x][z]->updateMesh();
        }
    }

    glClearColor(0.2f, 0.3f, 0.8f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // variables for deltaTime
    double previousTime = glfwGetTime();
    double deltaTime = 0.0f;

    // render loop
    while (!glfwWindowShouldClose(window)) {
        displayFPS();
        double currentTime = glfwGetTime();
        deltaTime = currentTime - previousTime;
        previousTime = currentTime;
        processInput(window, &camera, static_cast<float>(deltaTime));

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float scrRatio = static_cast<float>(g_scrWidth) / g_scrHeight;
        for (int x = 0; x < numChunksX; ++x) {
            for (int z = 0; z < numChunksZ; ++z) {
                chunks[x][z]->render(camera.getViewMatrix(), camera.getZoom(), scrRatio);
            }
        }

        // catch errors
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cout << "OpenGL Error: " << err << '\n';
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for (int x = 0; x < numChunksX; ++x) {
        for (int z = 0; z < numChunksZ; ++z) {
            delete chunks[x][z];
        }
    }
    glfwTerminate();
    return 0;
}
