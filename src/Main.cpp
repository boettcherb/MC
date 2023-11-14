#include "Constants.h"
#include "Shader.h"
#include "Camera.h"
#include "Texture.h"
#include "ChunkLoader.h"
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

// called by std::at_exit()
void close_app() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    database::close();
    glfwTerminate();
}

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
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

void render_imgui_window(ImGuiIO& io) {
    static bool show_demo_window = false;
    static bool show_another_window = true;
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*) &clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window) {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

    // Rendering
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

    glClearColor(CLEAR_R, CLEAR_G, CLEAR_B, 1.0f);
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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
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
    blockShader.addUniform3f("u4_bgColor", CLEAR_R, CLEAR_G, CLEAR_B);
    blockShader.addUniform1i("u5_renderDist", 16 * (LOAD_RADIUS - 3));
    
    int camX = (int) camera.getPosition().x / CHUNK_WIDTH - (camera.getPosition().x < 0);
    int camZ = (int) camera.getPosition().z / CHUNK_WIDTH - (camera.getPosition().z < 0);
    ChunkLoader chunkLoader(&blockShader, camX, camZ);

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

        render_imgui_window(io);

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
