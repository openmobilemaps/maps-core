// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from graphicsobjects.djinni

#pragma once

#include "Quad3dD.h"
#include "RectD.h"
#include "RenderingContextInterface.h"
#include "Vec3D.h"
#include <cstdint>
#include <memory>

class GraphicsObjectInterface;
class MaskingObjectInterface;
class TextureHolderInterface;
enum class TextureFilterType;

class Quad2dInterface {
public:
    virtual ~Quad2dInterface() = default;

    virtual void setFrame(const ::Quad3dD & frame, const ::RectD & textureCoordinates, const ::Vec3D & origin, bool is3d) = 0;

    virtual void setSubdivisionFactor(int32_t factor) = 0;

    virtual void setMinMagFilter(TextureFilterType filterType) = 0;

    virtual void loadTexture(const /*not-null*/ std::shared_ptr<::RenderingContextInterface> & context, const /*not-null*/ std::shared_ptr<TextureHolderInterface> & textureHolder) = 0;

    virtual void removeTexture() = 0;

    virtual /*not-null*/ std::shared_ptr<GraphicsObjectInterface> asGraphicsObject() = 0;

    virtual /*not-null*/ std::shared_ptr<MaskingObjectInterface> asMaskingObject() = 0;
};
