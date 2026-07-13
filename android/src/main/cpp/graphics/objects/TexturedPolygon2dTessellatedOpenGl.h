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

#include "BaseGraphicsObjectOpenGl.h"
#include "GraphicsObjectInterface.h"
#include "MaskingObjectInterface.h"
#include "OpenGlContext.h"
#include "SharedBytes.h"
#include "TessellatedRasterShaderOpenGl.h"
#include "TextureAttachment.h"
#include "TextureFilterType.h"
#include "opengl_wrapper.h"
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

class TextureHolderInterface;

class TexturedPolygon2dTessellatedOpenGl : public BaseGraphicsObjectOpenGl,
                                           public MaskingObjectInterface,
                                           public std::enable_shared_from_this<TexturedPolygon2dTessellatedOpenGl> {
  public:
    explicit TexturedPolygon2dTessellatedOpenGl(const std::shared_ptr<::TessellatedRasterShaderOpenGl> &shader);

    bool isReady() override;

    void setup(const std::shared_ptr<::RenderingContextInterface> &context) override;

    void clear() override;

    void render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                int64_t vpMatrix, int64_t mMatrix, const ::Vec3D &origin, bool isMasked,
                double screenPixelAsRealMeterFactor, bool isScreenSpaceCoords) override;

    void renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                      int64_t vpMatrix, int64_t mMatrix, const ::Vec3D &origin,
                      double screenPixelAsRealMeterFactor, bool isScreenSpaceCoords) override;

    void setVertices(const ::SharedBytes &vertices, const ::SharedBytes &indices, const ::Vec3D &origin, bool is3d);

    void setSubdivisionFactor(int32_t factor);

    void setMinMagFilter(TextureFilterType filterType);

    void loadTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                     const std::shared_ptr<TextureHolderInterface> &textureHolder);

    void loadDualTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                         const std::shared_ptr<TextureHolderInterface> &textureHolder,
                         const std::shared_ptr<TextureHolderInterface> &elevationHolder);

    void removeTexture();

    std::shared_ptr<GraphicsObjectInterface> asGraphicsObject() override;

    std::shared_ptr<MaskingObjectInterface> asMaskingObject();

    void setIsInverseMasked(bool inversed) override;

    void setDebugLabel(const std::string &label) override;

  private:
    void prepareGlData(int program);

    void removeGlBuffers();

    void prepareTextureDraw(int program);

    void updateScaledTextureCoordinates();

    std::shared_ptr<TessellatedRasterShaderOpenGl> shaderProgram;
    std::string programName;
    int program = 0;

    bool glDataBuffersGenerated = false;
    int mMatrixHandle = 0;
    int originOffsetHandle = 0;
    int subdivisionFactorHandle = 0;
    int originHandle = 0;
    int is3dHandle = 0;
    int positionHandle = -1;
    int frameCoordHandle = -1;
    int textureCoordinateHandle = -1;
    int skirtOffsetHandle = -1;
    int textureUniformHandle = 0;
    int elevationTextureUniformHandle = 0;
    int hasElevationTextureHandle = 0;
    GLuint vao = 0;
    GLuint vertexBuffer = 0;
    GLuint indexBuffer = 0;

    std::vector<GLfloat> vertices;
    std::vector<GLfloat> unscaledTextureCoords;
    std::vector<GLushort> indices;
    int vertexStride = 0;
    int textureCoordOffset = 0;

    Vec3D polygonOrigin = Vec3D(0.0, 0.0, 0.0);
    bool is3d = false;
    int32_t subdivisionFactor = 0;

    TextureAttachment textureAttachment;
    TextureAttachment elevationTextureAttachment;
    std::optional<TextureFilterType> textureFilterType = std::nullopt;

    bool dataReady = false;
    bool ready = false;
    std::recursive_mutex dataMutex;

    bool isMaskInversed = false;
};
