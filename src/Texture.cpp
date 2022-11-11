#include "Texture.h"

#include <glad/glad.h>
#include "stb_image/stb_image.h"

#include <iostream>

Texture::Texture(const std::string& filePath, unsigned int slot) : m_textureSlot{ slot } {
    // create and bind the texture
    glGenTextures(1, &m_textureID);
    bind();

    // texture filtering (for when texture is too large or small)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // texture wrapping (for when texture coordinates are outside of [0, 1])
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // the image data from stbi_load starts at the top left 
    // but OpenGL reads images from the bottom left
    stbi_set_flip_vertically_on_load(1);

    // use stbi_image library to convert the image into a char*
    // I will always set the number of channels to 4 (RGBA)
    // the third int (the Bytes Per Pixel of the original image) is not used
    int width, height, BPP;
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &BPP, 4);

    // if the conversion was successful, generate the texture
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        // a mipmap is used for large and complex textures on small or faraway objects
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    } else {
        std::cerr << "Failed to load texture at " << filePath << '\n';
    }

    unbind();
}

Texture::~Texture() {
    glDeleteTextures(1, &m_textureID);
}

void Texture::bind() const {
    glActiveTexture(GL_TEXTURE0 + m_textureSlot);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
}

void Texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

unsigned int Texture::getSlot() const {
    return m_textureSlot;
}
