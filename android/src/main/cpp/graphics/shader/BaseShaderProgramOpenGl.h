/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#define OMMShaderCode(...) std::string(#__VA_ARGS__)
#define OMMVersionedGlesShaderCode(version, ...) std::string("#version " #version "\n" #__VA_ARGS__)
#define OMMVersionedGlesShaderCodeWithFrameUBO(version, ...) std::string("#version " #version "\n") + FRAME_UBO_DEFINITION + ("\n" #__VA_ARGS__)

#include "Logger.h"
#include "ShaderProgramInterface.h"
#include "opengl_wrapper.h"
#include "BlendMode.h"
#include "OpenGlContext.h"

class BaseShaderProgramOpenGl: public ShaderProgramInterface {
public:
    static int loadShader(int type, std::string shaderCode);

    static void checkGlProgramLinking(GLuint program);

    static void setupFrameUniforms(GLuint frameUniformsBuffer, int64_t vpMatrix, const ::Vec3D & origin,
                                   double screenPixelAsRealMeterFactor, double timeFrameDeltaSeconds);

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual bool usesModelMatrix() override { return true; };

    virtual void setupGlObjects(const std::shared_ptr<::OpenGlContext> &context);

    virtual void clearGlObjects();

    virtual bool isRenderable() { return true; }

    virtual void setBlendMode(BlendMode blendMode) override;

    const static size_t FRAME_UBO_SIZE = sizeof(GLfloat) * 6 * 4; // mat4, vec4, vec2 (padded for std140!)
    const static GLuint FRAME_UBO_BINDING_POINT;
    const static std::string FRAME_UBO_DEFINITION;
protected:
    GLint frameUniformsBufferBlockIdx = GL_INVALID_INDEX;
    GLint program = GL_INVALID_INDEX;

    virtual std::string getVertexShader();

    virtual std::string getFragmentShader();

    BlendMode blendMode = BlendMode::NORMAL;

private:
    static std::vector<GLfloat> tempVec4FrameUniforms;
};
