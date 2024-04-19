#include "Constants.h"
#include "Shader.h"
#include "Player.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <cassert>
#include <iostream>

void initialize_HUD();
void resize_HUD(int width, int height);
void render_HUD(Shader* shader);
void render_imgui_window(ImGuiIO& io, const Player& player);

static int screen_width;
static int screen_height;
static unsigned int crosshair_VAO, crosshair_VBO;

static float crosshair_data[24] = {
    -0.025f, -0.025f, 0.0f / 16.0f, 0.0f / 16.0f,
     0.025f, -0.025f, 1.0f / 16.0f, 0.0f / 16.0f,
     0.025f,  0.025f, 1.0f / 16.0f, 1.0f / 16.0f,
     0.025f,  0.025f, 1.0f / 16.0f, 1.0f / 16.0f,
    -0.025f,  0.025f, 0.0f / 16.0f, 1.0f / 16.0f,
    -0.025f, -0.025f, 0.0f / 16.0f, 0.0f / 16.0f,
};

void initialize_HUD() {
    // initialize the width and height to be a square so the
    // crosshair is rendered as a square (not skewed)
    screen_width = screen_height = 700;

    // crosshair
    glGenVertexArrays(1, &crosshair_VAO);
    glGenBuffers(1, &crosshair_VBO);
    glBindVertexArray(crosshair_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, crosshair_VBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, (void*) 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, (void*) 8);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
}

void resize_HUD(int width, int height) {
    for (int i = 0; i < 6; ++i) {
        crosshair_data[i * 4] *= (float) screen_width / width;
        crosshair_data[i * 4 + 1] *= (float) screen_height / height;
    }

    screen_width = width;
    screen_height = height;

    glBindVertexArray(crosshair_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, crosshair_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshair_data), crosshair_data, GL_STATIC_DRAW);
}

void render_HUD(Shader* shader) {
    glBindVertexArray(crosshair_VAO);
    shader->bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render_imgui_window(ImGuiIO& io, const Player& player) {
    static bool show_demo_window = false;
    static ImVec4 color = ImVec4(0.2f, 0.3f, 0.8f, 1.0f);
    static int render_distance = Player::getRenderDist();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);
    // create imgui window with title
    ImGui::Begin("Debug Info");
    // display coordinates
    sglm::vec3 pos = player.getPosition();
    ImGui::Text("Position: x = %.2f, y = %.2f, z = %.2f", pos.x, pos.y, pos.z);
    // player's chunk
    auto [px, pz] = player.getPlayerChunk();
    ImGui::Text("Player Chunk: x = %d, z = %d", px, pz);
    // coordinates of block player is looking at
    if (player.hasViewRayIsect()) {
        int x = player.getViewRayIsect().x + CHUNK_WIDTH * player.getViewRayIsect().cx;
        int y = player.getViewRayIsect().y;
        int z = player.getViewRayIsect().z + CHUNK_WIDTH * player.getViewRayIsect().cz;
        ImGui::Text("Looking at the block at: x = %d, y = %d, z = %d", x, y, z);
    } else {
        ImGui::Text("Looking at the block at: None");
    }
    // update clear color
    ImVec4 prev = { color.x, color.y, color.z, color.w };
    ImGui::ColorEdit3("Clear Color", (float*) &color); // Edit 3 floats representing a color
    if (prev.x != color.x || prev.y != color.y || prev.z != color.z) {
        glClearColor(color.x, color.y, color.z, color.w);
    }
    // render distance
    int prev_dist = render_distance;
    ImGui::SliderInt("render distance", &render_distance, 1, 32);
    if (prev_dist != render_distance) {
        Player::setRenderDist(render_distance);
    }
    // percentage of subchunks rendered
    auto [rendered, total] = player.chunks_rendered;
    ImGui::Text("SubChunks rendered: %d, total: %d (%.2f%%)",
                rendered, total, (float) rendered / total * 100.0f);
    // fov
    ImGui::Text("FOV: %.2f", player.getFOV());
    // display fps
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / io.Framerate, io.Framerate);
    ImGui::Checkbox("Demo Window", &show_demo_window);

    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
