#include "Constants.h"
#include "Shader.h"
#include "Player.h"
#include "Texture.h"
#include "World.h"
#include "BlockInfo.h"
#include "Database.h"

#include <glad/glad.h>
#include <GLFW/GLFW3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <iostream>
#include <cstdlib>

// from UI.cpp
void initialize_HUD();
void resize_HUD(int width, int height);
void render_HUD(Shader* shader);
void render_imgui_window(ImGuiIO& io, const Player& player);

static Player player = Player({0.0f, 80.0f, 0.0f}, (float) INITIAL_SCREEN_WIDTH / INITIAL_SCREEN_HEIGHT);
static bool mouse_captured = false;
static bool mine_block = false;
static bool f3_opened = false;

// called by std::at_exit()
static void close_app() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    database::close();
    glfwTerminate();
}

static std::pair<int, int> get_screen_size() {
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    return { mode->width, mode->height };
}

static void glfw_error_callback(int error, const char* description) {
    std::cout << "GLFW Error " << error << ": " << description << '\n';
}

static void window_size_callback(GLFWwindow* /* window */, int width, int height) {
    player.setAspectRatio((float) width / height);
    glViewport(0, 0, width, height);
    resize_HUD(width, height);
}

static void mouse_motion_callback(GLFWwindow* /* window */, double xpos, double ypos) {
    if (mouse_captured) {
        player.look((float) xpos, (float) ypos);
    }
}

static void mouse_button_callback(GLFWwindow* /* window */, int button, int action, int /* mods */) {
    if (mouse_captured) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            mine_block = true;
        }
    }
}

static void scroll_callback(GLFWwindow* /* window */, double /* offsetX */, double offsetY) {
    if (mouse_captured) {
        player.setFOV((float) offsetY);
    }
}

static void key_callback(GLFWwindow* window, int key, int /* scancode */, int action, int /* mods */) {
    // If the escape key is pressed, close the window.
    // If F2 is pressed, toggle capturing/releasing the mouse
    // If F3 is pressed, toggle opening the debug window
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    else if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
        mouse_captured = !mouse_captured;
        if (mouse_captured) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    else if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
        f3_opened = !f3_opened;
    }
}

static void processInput(GLFWwindow* window, float deltaTime) {
    // WASD for the camera
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        player.move(Movement::FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        player.move(Movement::BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        player.move(Movement::LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        player.move(Movement::RIGHT, deltaTime);
    }
}

int main() {
    std::atexit(close_app);

    // initialize GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // // glfwWindowHint(GLFW_SAMPLES, 1); // anti-aliasing is causing lines between blocks

    // create the main window
    //auto [scr_width, scr_height] = get_screen_size();
    //GLFWwindow* window = glfwCreateWindow(scr_width, scr_height, WINDOW_TITLE,
    //                                      glfwGetPrimaryMonitor(), nullptr);
    int scr_width = 800, scr_height = 600;
    get_screen_size();
    GLFWwindow* window = glfwCreateWindow(scr_width, scr_height, WINDOW_TITLE,
                                          nullptr, nullptr);



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

    glClearColor(0.2f, 0.3f, 0.8f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);

    // tell GLFW to capture our mouse cursor
    if (mouse_captured)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // enable VSync (tie the FPS to your monitor's refresh rate)
    glfwSwapInterval(1);

    initialize_HUD();
    window_size_callback(nullptr, scr_width, scr_height);
    database::initialize();
    Block::setBlockData();

    // initialize imgui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");



    /////////////////////////////////////////////////////////////////////////////////



    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << '\n';
    std::cout << "Starting Application...\n";

    Shader blockShader(BLOCK_VERTEX, BLOCK_FRAGMENT);
    Shader uiShader(UI_VERTEX, UI_FRAGMENT);
    Texture textureSheet(TEXTURE_SHEET, 0);
    blockShader.addTexture(&textureSheet, "u3_texture");
    uiShader.addTexture(&textureSheet, "u3_texture");
    
    World chunkLoader(&blockShader, &player);

    // variables for deltaTime
    double previousTime = glfwGetTime();
    double deltaTime = 0.0f;

    // render loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        double currentTime = glfwGetTime();
        deltaTime = currentTime - previousTime;
        previousTime = currentTime;
        if (mouse_captured) {
            processInput(window, (float) deltaTime);
        }
        chunkLoader.update(mine_block);
        mine_block = false;
        chunkLoader.renderAll();
        render_HUD(&uiShader);

        if (f3_opened)
            render_imgui_window(io, player);

#ifndef NDEBUG
        // catch errors
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cout << "OpenGL Error (in main): " << err << '\n';
        }
#endif

        glfwSwapBuffers(window);
    }
    return 0;
}
