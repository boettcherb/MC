#include "Shader.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

void initialize_HUD(int width, int height);
void resize_HUD(int width, int height);
void render_HUD(Shader* shader);

static int screen_width;
static int screen_height;
static unsigned int crosshair_VAO;
static unsigned int crosshair_VBO;

static float crosshair_data[24] = {
    -0.025f, -0.025f, 0.0f / 16.0f, 13.0f / 16.0f,
     0.025f, -0.025f, 1.0f / 16.0f, 13.0f / 16.0f,
     0.025f,  0.025f, 1.0f / 16.0f, 14.0f / 16.0f,
     0.025f,  0.025f, 1.0f / 16.0f, 14.0f / 16.0f,
    -0.025f,  0.025f, 0.0f / 16.0f, 14.0f / 16.0f,
    -0.025f, -0.025f, 0.0f / 16.0f, 13.0f / 16.0f,
};

void initialize_HUD(int width, int height) {
    // initialize the width and height to be a square so the
    // crosshair is rendered as a square (not skewed)
    screen_width = screen_height = 800;

    glGenVertexArrays(1, &crosshair_VAO);
    glGenBuffers(1, &crosshair_VBO);
    glBindVertexArray(crosshair_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, crosshair_VBO);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, (void*) 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, (void*) 8);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    resize_HUD(width, height);
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
