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

#include "GraphicsObjectInterface.h"
#include "MaskingObjectInterface.h"
#include "OpenGlContext.h"
#include "IcosahedronInterface.h"
#include "ShaderProgramInterface.h"
#include "opengl_wrapper.h"
#include <mutex>

class IcosahedronOpenGl : public GraphicsObjectInterface,
                          public IcosahedronInterface,
                          public std::enable_shared_from_this<IcosahedronOpenGl> {
public:
    IcosahedronOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader);

    ~IcosahedronOpenGl(){};

    virtual bool isReady() override;

    virtual void setup(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void clear() override;

    virtual void render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass, int64_t vpMatrix,
           int64_t mMatrix, const ::Vec3D &origin, bool isMasked, double screenPixelAsRealMeterFactor) override;

    virtual void setVertices(const ::SharedBytes & vertices, const ::SharedBytes & indices, const ::Vec3D & origin) override;

    virtual std::shared_ptr<GraphicsObjectInterface> asGraphicsObject() override;

    virtual void setIsInverseMasked(bool inversed) override;

    void setDebugLabel(const std::string &label) override;

protected:
    void prepareGlData(int program);

    void removeGlBuffers();

    std::shared_ptr<ShaderProgramInterface> shaderProgram;
    std::string programName;
    int program = 0;

    int vpMatrixHandle;
    int mMatrixHandle;
    int positionHandle;
    int valueHandle;
    GLuint vao;
    GLuint vertexBuffer;
    std::vector<GLfloat> vertices;
    GLuint indexBuffer;
    std::vector<GLuint> indices;
    bool glDataBuffersGenerated = false;


    bool dataReady = false;
    bool ready = false;
    std::recursive_mutex dataMutex;

    bool isMaskInversed = false;
};
