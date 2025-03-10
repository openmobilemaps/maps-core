// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from graphicsobjects.djinni

#pragma once

#include "Quad2dD.h"
#include "RenderingContextInterface.h"
#include "SharedBytes.h"
#include "Vec3D.h"
#include <cstdint>
#include <memory>

class GraphicsObjectInterface;
class MaskingObjectInterface;
class TextureHolderInterface;

class Quad2dStretchedInstancedInterface {
public:
    virtual ~Quad2dStretchedInstancedInterface() = default;

    virtual void setFrame(const ::Quad2dD & frame, const ::Vec3D & origin, bool is3d) = 0;

    virtual void setInstanceCount(int32_t count) = 0;

    virtual void setPositions(const ::SharedBytes & positions) = 0;

    virtual void setScales(const ::SharedBytes & scales) = 0;

    virtual void setRotations(const ::SharedBytes & rotations) = 0;

    virtual void setAlphas(const ::SharedBytes & values) = 0;

    /**
     * stretch infos consists of:
     * scaleX: f32;
     *  all stretch infos are between 0 and 1
     * stretchX0Begin: f32;
     * stretchX0End: f32;
     * stretchX1Begin: f32;
     * stretchX1End: f32;
     * scaleY: f32;
     * stretchY0Begin: f32;
     * stretchY0End: f32;
     * stretchY1Begin: f32;
     * stretchY1End: f32;
     * so a total of 10 floats for each instance
     */
    virtual void setStretchInfos(const ::SharedBytes & values) = 0;

    virtual void setTextureCoordinates(const ::SharedBytes & textureCoordinates) = 0;

    virtual void loadTexture(const /*not-null*/ std::shared_ptr<::RenderingContextInterface> & context, const /*not-null*/ std::shared_ptr<TextureHolderInterface> & textureHolder) = 0;

    virtual void removeTexture() = 0;

    virtual /*not-null*/ std::shared_ptr<GraphicsObjectInterface> asGraphicsObject() = 0;

    virtual /*not-null*/ std::shared_ptr<MaskingObjectInterface> asMaskingObject() = 0;
};
