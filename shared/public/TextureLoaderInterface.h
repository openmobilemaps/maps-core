// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from loader.djinni

#pragma once

#include <string>

struct TextureLoaderResult;

class TextureLoaderInterface {
public:
    virtual ~TextureLoaderInterface() {}

    virtual TextureLoaderResult loadTexture(const std::string & url) = 0;
};
