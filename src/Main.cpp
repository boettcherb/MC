#include "Constants.h"
#include "Shader.h"
#include "Camera.h"
#include "Texture.h"
#include "ChunkLoader.h"
#include "Database.h"
#include <glad/glad.h>
#include <GLFW/GLFW3.h>
#include <iostream>
#include <cstdlib>

// from UI.cpp
void initialize_HUD(int width, int height);
void resize_HUD(int width, int height);
void render_HUD(Shader* shader);

static Camera camera(PLAYER_INITIAL_POSITION, (float) INITIAL_SCREEN_WIDTH / INITIAL_SCREEN_HEIGHT);
static bool mouse_captured = true;
static bool mine_block = false;

// called by std::at_exit()
void close_app() {
    database::close();
    glfwTerminate();
}

void window_size_callback(GLFWwindow* /* window */, int width, int height) {
    camera.processNewAspectRatio((float) width / height);
    glViewport(0, 0, width, height);
    resize_HUD(width, height);
}

void mouse_motion_callback(GLFWwindow* /* window */, double xpos, double ypos) {
    if (mouse_captured) {
        camera.processMouseMovement((float) xpos, (float) ypos);
    }
}

void mouse_button_callback(GLFWwindow* /* window */, int button, int action, int /* mods */) {
    if (mouse_captured) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            mine_block = true;
        }
    }
}

void scroll_callback(GLFWwindow* /* window */, double /* offsetX */, double offsetY) {
    if (mouse_captured) {
        camera.processMouseScroll((float) offsetY);
    }
}

void key_callback(GLFWwindow* window, int key, int /* scancode */, int action, int mods) {
    // If the escape key is pressed, close the window. If the escape key is
    // pressed while shift is pressed, toggle holding / releasing the mouse.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (mods & GLFW_MOD_SHIFT) {
            if (mouse_captured) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                mouse_captured = false;
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                mouse_captured = true;
            }
        } else {
            glfwSetWindowShouldClose(window, true);
        }
    }
}

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
    std::atexit(close_app);

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
    GLFWwindow* window = glfwCreateWindow(INITIAL_SCREEN_WIDTH, INITIAL_SCREEN_HEIGHT,
                                          WINDOW_TITLE, nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetCursorPosCallback(window, mouse_motion_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    initialize_HUD(INITIAL_SCREEN_WIDTH, INITIAL_SCREEN_HEIGHT);
    database::initialize();

    glClearColor(CLEAR_R, CLEAR_G, CLEAR_B, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);

    // tell GLFW to capture our mouse cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // enable VSync (tie the FPS to your monitor's refresh rate)
    glfwSwapInterval(1);



    /////////////////////////////////////////////////////////////////////////////////



    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << '\n';
    std::cout << "Starting Application...\n";

    Shader blockShader(BLOCK_VERTEX, BLOCK_FRAGMENT);
    Shader uiShader(UI_VERTEX, UI_FRAGMENT);
    Texture textureSheet(TEXTURE_SHEET, 0);
    blockShader.addUniform3f("u4_bgColor", CLEAR_R, CLEAR_G, CLEAR_B);
    blockShader.addUniform1i("u5_renderDist", 16 * (LOAD_RADIUS - 3));
    blockShader.addTexture(&textureSheet, "u3_texture");
    uiShader.addTexture(&textureSheet, "u3_texture");
    
    int camX = (int) camera.getPosition().x / CHUNK_WIDTH - (camera.getPosition().x < 0);
    int camZ = (int) camera.getPosition().z / CHUNK_WIDTH - (camera.getPosition().z < 0);
    ChunkLoader chunkLoader(&blockShader, camX, camZ);

    // variables for deltaTime
    double previousTime = glfwGetTime();
    double deltaTime = 0.0f;

    // render loop
    while (!glfwWindowShouldClose(window)) {
        displayFPS();
        double currentTime = glfwGetTime();
        deltaTime = currentTime - previousTime;
        previousTime = currentTime;
        if (mouse_captured) {
            processInput(window, (float) deltaTime);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        chunkLoader.update(camera, mine_block);
        mine_block = false;
        chunkLoader.renderAll(camera);
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
    return 0;
}
