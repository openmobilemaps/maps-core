#include "OpenGlHelper.h"

// Generate mipmaps, mainly intended for sprites.
void OpenGlHelper::generateMipmap(uint32_t texturePointer) 
{
    const GLint targetMaxLevels = 4;

    glBindTexture(GL_TEXTURE_2D, texturePointer);
    GLint currMaxLevels;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &currMaxLevels);
    // The default value is 1000. By setting this we know that we already created mipmap for this texture.
    if(currMaxLevels == targetMaxLevels) {
        return;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, targetMaxLevels);
    glGenerateMipmap(GL_TEXTURE_2D);
}
