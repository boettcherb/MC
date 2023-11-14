#include "Constants.h"
#include "Shader.h"
#include "Camera.h"
#include "Texture.h"
#include "World.h"
#include "Database.h"

#include <glad/glad.h>
#include <GLFW/GLFW3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <iostream>
#include <cstdlib>

// from UI.cpp
void initialize_HUD(int width, int height);
void resize_HUD(int width, int height);
void render_HUD(Shader* shader);

static Camera camera(PLAYER_INITIAL_POSITION, (float) INITIAL_SCREEN_WIDTH / INITIAL_SCREEN_HEIGHT);
static bool mouse_captured = true;
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

static void glfw_error_callback(int error, const char* description) {
    std::cout << "GLFW Error " << error << ": " << description << '\n';
}

static void window_size_callback(GLFWwindow* /* window */, int width, int height) {
    camera.processNewAspectRatio((float) width / height);
    glViewport(0, 0, width, height);
    resize_HUD(width, height);
}

static void mouse_motion_callback(GLFWwindow* /* window */, double xpos, double ypos) {
    if (mouse_captured) {
        camera.processMouseMovement((float) xpos, (float) ypos);
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
        camera.processMouseScroll((float) offsetY);
    }
}

static void key_callback(GLFWwindow* window, int key, int /* scancode */, int action, int mods) {
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
    if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
        f3_opened = !f3_opened;
        if (f3_opened) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            mouse_captured = false;
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            mouse_captured = true;

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

static void render_imgui_window(ImGuiIO& io, Shader& shader) {
    static bool show_demo_window = false;
    static ImVec4 color = ImVec4(0.2f, 0.3f, 0.8f, 1.0f);
    static int render_distance = 16 * (LOAD_RADIUS - 3);

    if (!f3_opened)
        return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);
    // create imgui window with title
    ImGui::Begin("Debug Info");
    // display coordinates
    sglm::vec3 pos = camera.getPosition();
    ImGui::Text("Position: x = %.2f, y = %.2f, z = %.2f", pos.x, pos.y, pos.z);
    // update clear color
    ImVec4 prev = { color.x, color.y, color.z, color.w };
    ImGui::ColorEdit3("Clear Color", (float*) &color); // Edit 3 floats representing a color
    if (prev.x != color.x || prev.y != color.y || prev.z != color.z) {
        glClearColor(color.x, color.y, color.z, color.w);
        shader.addUniform3f("u4_bgColor", color.x, color.y, color.z);
    }
    // render distance
    int prev_dist = render_distance;
    ImGui::SliderInt("render distance", &render_distance, 0, 32 * (LOAD_RADIUS - 3));
    if (prev_dist != render_distance) {
        shader.addUniform1i("u5_renderDist", render_distance);
    }
    // display fps
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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

    glClearColor(0.2f, 0.3f, 0.8f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);

    // tell GLFW to capture our mouse cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // enable VSync (tie the FPS to your monitor's refresh rate)
    glfwSwapInterval(1);

    initialize_HUD(INITIAL_SCREEN_WIDTH, INITIAL_SCREEN_HEIGHT);
    database::initialize();

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
    blockShader.addUniform3f("u4_bgColor", 0.2f, 0.3f, 0.8f);
    blockShader.addUniform1i("u5_renderDist", 16 * (LOAD_RADIUS - 3));
    
    int camChunkX = (int) camera.getPosition().x / CHUNK_WIDTH - (camera.getPosition().x < 0);
    int camChunkZ = (int) camera.getPosition().z / CHUNK_WIDTH - (camera.getPosition().z < 0);
    World chunkLoader(&blockShader, camChunkX, camChunkZ);

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
        chunkLoader.update(camera, mine_block);
        mine_block = false;
        chunkLoader.renderAll(camera);
        render_HUD(&uiShader);

        render_imgui_window(io, blockShader);

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
