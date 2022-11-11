#ifndef TEXTURE_H_INCLUDED
#define TEXTURE_H_INCLUDED

#include <string>

class Texture {
    unsigned int m_textureID;
    unsigned int m_textureSlot;

public:
    Texture(const std::string& filePath, unsigned int slot);
    ~Texture();

    void bind() const;
    void unbind() const;
    unsigned int getSlot() const;
};

#endif
