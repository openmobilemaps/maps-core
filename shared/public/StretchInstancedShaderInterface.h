// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from shader.djinni

#pragma once

#include <memory>

class ShaderProgramInterface;

class StretchInstancedShaderInterface {
public:
    virtual ~StretchInstancedShaderInterface() = default;

    virtual /*not-null*/ std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface() = 0;
};
