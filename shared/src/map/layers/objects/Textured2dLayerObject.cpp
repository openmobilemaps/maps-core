/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Textured2dLayerObject.h"
#include "DateHelper.h"
#include "DoubleAnimation.h"
#include "EarcutVec2D.h"
#include "RasterStyleAnimation.h"
#include "RenderObject.h"
#include "SharedBytes.h"
#include "TessellationSettings.h"
#include <algorithm>
#include <cmath>
#include <cassert>
#include <limits>

Textured2dLayerObject::Textured2dLayerObject(std::shared_ptr<Quad2dInterface> quad,
                                             const std::shared_ptr<AlphaShaderInterface> &shader,
                                             const std::shared_ptr<MapInterface> &mapInterface,
                                             bool is3d)
        : quad(quad), texturedPolygon(nullptr), shader(shader), rasterShader(nullptr), mapInterface(mapInterface),
          conversionHelper(mapInterface->getCoordinateConverterHelper()),
          renderConfig(std::make_shared<RenderConfig>(quad->asGraphicsObject(), 0)), graphicsObject(quad->asGraphicsObject()),
          renderObject(std::make_shared<RenderObject>(graphicsObject)),
          is3d(is3d) {}


Textured2dLayerObject::Textured2dLayerObject(std::shared_ptr<Quad2dInterface> quad,
                                             const std::shared_ptr<RasterShaderInterface> &rasterShader,
                                             const std::shared_ptr<MapInterface> &mapInterface,
                                             bool is3d)
        : quad(quad), texturedPolygon(nullptr), shader(nullptr), rasterShader(rasterShader), mapInterface(mapInterface),
          conversionHelper(mapInterface->getCoordinateConverterHelper()),
          renderConfig(std::make_shared<RenderConfig>(quad->asGraphicsObject(), 0)), graphicsObject(quad->asGraphicsObject()),
          renderObject(std::make_shared<RenderObject>(graphicsObject)),
          is3d(is3d) {}


Textured2dLayerObject::Textured2dLayerObject(std::shared_ptr<Quad2dInterface> quad,
                                             const std::shared_ptr<MapInterface> &mapInterface,
                                             bool is3d)
        : quad(quad), texturedPolygon(nullptr), shader(nullptr), rasterShader(nullptr), mapInterface(mapInterface),
          conversionHelper(mapInterface->getCoordinateConverterHelper()),
          renderConfig(std::make_shared<RenderConfig>(quad->asGraphicsObject(), 0)), graphicsObject(quad->asGraphicsObject()),
          renderObject(std::make_shared<RenderObject>(graphicsObject)),
          is3d(is3d) {}

Textured2dLayerObject::Textured2dLayerObject(std::shared_ptr<TexturedPolygonInterface> texturedPolygon,
                                             const std::shared_ptr<AlphaShaderInterface> &shader,
                                             const std::shared_ptr<MapInterface> &mapInterface,
                                             bool is3d)
        : quad(nullptr), texturedPolygon(texturedPolygon), shader(shader), rasterShader(nullptr), mapInterface(mapInterface),
          conversionHelper(mapInterface->getCoordinateConverterHelper()),
          renderConfig(std::make_shared<RenderConfig>(texturedPolygon->asGraphicsObject(), 0)),
          graphicsObject(texturedPolygon->asGraphicsObject()),
          renderObject(std::make_shared<RenderObject>(graphicsObject)),
          is3d(is3d) {}

Textured2dLayerObject::Textured2dLayerObject(std::shared_ptr<TexturedPolygonInterface> texturedPolygon,
                                             const std::shared_ptr<RasterShaderInterface> &rasterShader,
                                             const std::shared_ptr<MapInterface> &mapInterface,
                                             bool is3d)
        : quad(nullptr), texturedPolygon(texturedPolygon), shader(nullptr), rasterShader(rasterShader), mapInterface(mapInterface),
          conversionHelper(mapInterface->getCoordinateConverterHelper()),
          renderConfig(std::make_shared<RenderConfig>(texturedPolygon->asGraphicsObject(), 0)),
          graphicsObject(texturedPolygon->asGraphicsObject()),
          renderObject(std::make_shared<RenderObject>(graphicsObject)),
          is3d(is3d) {}

Textured2dLayerObject::Textured2dLayerObject(std::shared_ptr<TexturedPolygonInterface> texturedPolygon,
                                             const std::shared_ptr<MapInterface> &mapInterface,
                                             bool is3d)
        : quad(nullptr), texturedPolygon(texturedPolygon), shader(nullptr), rasterShader(nullptr), mapInterface(mapInterface),
          conversionHelper(mapInterface->getCoordinateConverterHelper()),
          renderConfig(std::make_shared<RenderConfig>(texturedPolygon->asGraphicsObject(), 0)),
          graphicsObject(texturedPolygon->asGraphicsObject()),
          renderObject(std::make_shared<RenderObject>(graphicsObject)),
          is3d(is3d) {}

void Textured2dLayerObject::setRectCoord(const ::RectCoord &rectCoord) {
    setRectCoord(rectCoord, 0.0);
}

void Textured2dLayerObject::setRectCoord(const ::RectCoord &rectCoord, double overlapFactor) {
    auto width = rectCoord.bottomRight.x - rectCoord.topLeft.x;
    auto height = rectCoord.bottomRight.y - rectCoord.topLeft.y;
    const double xOverlap = width * overlapFactor;
    const double yOverlap = height * overlapFactor;
    setPosition(
        Coord(rectCoord.topLeft.systemIdentifier, rectCoord.topLeft.x - xOverlap, rectCoord.topLeft.y - yOverlap, rectCoord.topLeft.z),
        width + 2.0 * xOverlap,
        height + 2.0 * yOverlap);
}

void Textured2dLayerObject::setPosition(const ::Coord &coord, double width, double height) {
    setPositions(QuadCoord(coord, Coord(coord.systemIdentifier, coord.x + width, coord.y, coord.z),
                           Coord(coord.systemIdentifier, coord.x + width, coord.y + height, coord.z),
                           Coord(coord.systemIdentifier, coord.x, coord.y + height, coord.z)));
}

void Textured2dLayerObject::setPositions(const ::QuadCoord &coords) {
    QuadCoord renderCoords = conversionHelper->convertQuadToRenderSystem(coords);

    const double cx = (renderCoords.bottomRight.x + renderCoords.topLeft.x) / 2.0;
    const double cy = (renderCoords.bottomRight.y + renderCoords.topLeft.y) / 2.0;
    const double cz = 0.0;

    auto origin = Vec3D(cx, cy, cz);

    if (is3d) {
        origin.x = 1.0 * sin(cy) * cos(cx);
        origin.y = 1.0 * cos(cy);
        origin.z = -1.0 * sin(cy) * sin(cx);
    }

    auto transform = [](const Coord coordinate) -> Vec3D {
        const double x = coordinate.x;
        const double y = coordinate.y;
        const double z = coordinate.z;
        return Vec3D(x, y, z);
    };

    setFrame(Quad3dD(transform(renderCoords.topLeft),
                     transform(renderCoords.topRight),
                     transform(renderCoords.bottomRight),
                     transform(renderCoords.bottomLeft)), origin);
}

void Textured2dLayerObject::setFrame(const ::Quad3dD &frame, const ::Vec3D & origin) {
    assert(quad != nullptr);
    quad->setFrame(frame, RectD(0, 0, 1, 1), origin, is3d);
}

void Textured2dLayerObject::setPolygons(const std::vector<::PolygonCoord> &polygons, const ::RectCoord &textureBounds) {
    setPolygons(polygons, textureBounds, 0.0);
}

void Textured2dLayerObject::setPolygons(const std::vector<::PolygonCoord> &polygons, const ::RectCoord &textureBounds, double overlapFactor) {
    auto expandCoord = [&textureBounds, overlapFactor](const Coord &coord, bool inward) {
        if (overlapFactor == 0.0) {
            return coord;
        }

        const double centerX = (textureBounds.topLeft.x + textureBounds.bottomRight.x) * 0.5;
        const double centerY = (textureBounds.topLeft.y + textureBounds.bottomRight.y) * 0.5;
        const double scale = inward ? std::max(0.0, 1.0 - 2.0 * overlapFactor) : 1.0 + 2.0 * overlapFactor;
        return Coord(coord.systemIdentifier,
                     centerX + (coord.x - centerX) * scale,
                     centerY + (coord.y - centerY) * scale,
                     coord.z);
    };

    const auto expandedTextureBounds = RectCoord(expandCoord(textureBounds.topLeft, false),
                                                expandCoord(textureBounds.bottomRight, false));
    const auto boundsQuad = QuadCoord(
        expandedTextureBounds.topLeft,
        Coord(expandedTextureBounds.topLeft.systemIdentifier, expandedTextureBounds.bottomRight.x, expandedTextureBounds.topLeft.y, expandedTextureBounds.topLeft.z),
        expandedTextureBounds.bottomRight,
        Coord(expandedTextureBounds.topLeft.systemIdentifier, expandedTextureBounds.topLeft.x, expandedTextureBounds.bottomRight.y, expandedTextureBounds.bottomRight.z));
    const auto renderBounds = conversionHelper->convertQuadToRenderSystem(boundsQuad);

    const double cx = (renderBounds.bottomRight.x + renderBounds.topLeft.x) / 2.0;
    const double cy = (renderBounds.bottomRight.y + renderBounds.topLeft.y) / 2.0;
    const double cz = 0.0;

    auto origin = Vec3D(cx, cy, cz);
    if (is3d) {
        origin.x = 1.0 * sin(cy) * cos(cx);
        origin.y = 1.0 * cos(cy);
        origin.z = -1.0 * sin(cy) * sin(cx);
    }

    const double textureWidth = expandedTextureBounds.bottomRight.x - expandedTextureBounds.topLeft.x;
    const double textureHeight = expandedTextureBounds.bottomRight.y - expandedTextureBounds.topLeft.y;
    const double renderTileWidth = std::abs(renderBounds.bottomRight.x - renderBounds.topLeft.x);
    const double renderTileHeight = std::abs(renderBounds.bottomRight.y - renderBounds.topLeft.y);
    const double terrainSkirtOffset = is3d ? std::min(std::max(renderTileWidth, renderTileHeight) / 5.0, 0.00015) : 0.0;

    std::vector<uint16_t> indices;
    std::vector<Vec2D> renderVertices;
    std::vector<Vec2D> textureVertices;
    std::vector<float> skirtOffsets;
    mapbox::detail::Earcut<int32_t> earcutter;

    const bool useTessellatedLayout =
#if HARDWARE_TESSELLATION_SUPPORTED
        is3d;
#else
        false;
#endif

    auto textureCoord = [&expandedTextureBounds, textureWidth, textureHeight](const Coord &coord) {
        const double u = textureWidth == 0.0 ? 0.0 : (coord.x - expandedTextureBounds.topLeft.x) / textureWidth;
        const double v = textureHeight == 0.0 ? 0.0 : (coord.y - expandedTextureBounds.topLeft.y) / textureHeight;
        return Vec2D(u, v);
    };

    for (const auto &polygon : polygons) {
        std::vector<std::vector<Vec2D>> renderCoords;
        std::vector<std::vector<Vec2D>> textureCoords;

        std::vector<Vec2D> polygonCoords;
        std::vector<Vec2D> polygonTextureCoords;
        for (const auto &mapCoord : polygon.positions) {
            const auto expandedCoord = expandCoord(mapCoord, false);
            const auto renderCoord = conversionHelper->convertToRenderSystem(expandedCoord);
            polygonCoords.emplace_back(renderCoord.x, renderCoord.y);
            polygonTextureCoords.push_back(textureCoord(expandedCoord));
        }
        renderCoords.push_back(std::move(polygonCoords));
        textureCoords.push_back(std::move(polygonTextureCoords));

        for (const auto &hole : polygon.holes) {
            std::vector<Vec2D> holeCoords;
            std::vector<Vec2D> holeTextureCoords;
            for (const auto &mapCoord : hole) {
                const auto expandedCoord = expandCoord(mapCoord, true);
                const auto renderCoord = conversionHelper->convertToRenderSystem(expandedCoord);
                holeCoords.emplace_back(renderCoord.x, renderCoord.y);
                holeTextureCoords.push_back(textureCoord(expandedCoord));
            }
            renderCoords.push_back(std::move(holeCoords));
            textureCoords.push_back(std::move(holeTextureCoords));
        }

        earcutter(renderCoords);
        const auto polygonVertexOffset = renderVertices.size();
        const std::vector<int32_t> curIndices = std::move(earcutter.indices);
        for (const auto &index : curIndices) {
            const auto vertexIndex = polygonVertexOffset + static_cast<size_t>(index);
            if (vertexIndex <= std::numeric_limits<uint16_t>::max()) {
                indices.push_back(static_cast<uint16_t>(vertexIndex));
            }
        }

        std::vector<std::pair<uint16_t, uint16_t>> skirtSegments;
        size_t ringVertexOffset = 0;
        for (size_t ringIndex = 0; ringIndex < renderCoords.size(); ++ringIndex) {
            const auto &ring = renderCoords[ringIndex];
            const auto &textureRing = textureCoords[ringIndex];
            const auto ringStartIndex = polygonVertexOffset + ringVertexOffset;

            for (size_t i = 0; i < ring.size(); ++i) {
                renderVertices.push_back(ring[i]);
                textureVertices.push_back(textureRing[i]);
                skirtOffsets.push_back(0.0f);
            }
            ringVertexOffset += ring.size();

            if (!useTessellatedLayout || terrainSkirtOffset <= 0.0 || ring.size() < 2) {
                continue;
            }

            for (size_t i = 0; i < ring.size(); ++i) {
                const size_t next = (i + 1) % ring.size();
                const auto edgeDx = ring[i].x - ring[next].x;
                const auto edgeDy = ring[i].y - ring[next].y;
                if ((edgeDx * edgeDx + edgeDy * edgeDy) <= 0.0) {
                    continue;
                }
                const auto topA = ringStartIndex + i;
                const auto topB = ringStartIndex + next;
                if (topA > std::numeric_limits<uint16_t>::max() ||
                    topB > std::numeric_limits<uint16_t>::max()) {
                    continue;
                }
                skirtSegments.emplace_back(static_cast<uint16_t>(topA), static_cast<uint16_t>(topB));
            }
        }

        for (const auto &[topA, topB] : skirtSegments) {
            if (renderVertices.size() + 2 > std::numeric_limits<uint16_t>::max()) {
                break;
            }

            const auto skirtA = static_cast<uint16_t>(renderVertices.size());
            renderVertices.push_back(renderVertices[topA]);
            textureVertices.push_back(textureVertices[topA]);
            skirtOffsets.push_back(static_cast<float>(terrainSkirtOffset));

            const auto skirtB = static_cast<uint16_t>(renderVertices.size());
            renderVertices.push_back(renderVertices[topB]);
            textureVertices.push_back(textureVertices[topB]);
            skirtOffsets.push_back(static_cast<float>(terrainSkirtOffset));

            indices.push_back(topA);
            indices.push_back(topB);
            indices.push_back(skirtB);
            indices.push_back(topA);
            indices.push_back(skirtB);
            indices.push_back(skirtA);
        }
    }

    std::vector<float> vertices;
    vertices.reserve(renderVertices.size() * (useTessellatedLayout ? 12 : 8));

    for (size_t i = 0; i < renderVertices.size(); ++i) {
        const auto &position = renderVertices[i];
        const auto &texture = textureVertices[i];
        const auto skirtOffset = skirtOffsets[i];

        const double x = is3d ? (1.0 * sin(position.y) * cos(position.x) - origin.x) : position.x - origin.x;
        const double y = is3d ? (1.0 * cos(position.y) - origin.y) : position.y - origin.y;
        const double z = is3d ? (-1.0 * sin(position.y) * sin(position.x) - origin.z) : 0.0;

        vertices.push_back(static_cast<float>(x));
        vertices.push_back(static_cast<float>(y));
        vertices.push_back(static_cast<float>(z));
        vertices.push_back(0.0f);
        if (useTessellatedLayout) {
            vertices.push_back(static_cast<float>(position.x));
            vertices.push_back(static_cast<float>(position.y));
            vertices.push_back(static_cast<float>(texture.x));
            vertices.push_back(static_cast<float>(texture.y));
            vertices.push_back(skirtOffset);
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        } else {
            vertices.push_back(static_cast<float>(texture.x));
            vertices.push_back(static_cast<float>(texture.y));
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }

    auto vertexBytes = SharedBytes((int64_t)vertices.data(), (int32_t)vertices.size(), (int32_t)sizeof(float));
    auto indexBytes = SharedBytes((int64_t)indices.data(), (int32_t)indices.size(), (int32_t)sizeof(uint16_t));
    assert(texturedPolygon != nullptr);
    if (texturedPolygon) {
        texturedPolygon->setVertices(vertexBytes, indexBytes, origin, is3d);
    }
}

void Textured2dLayerObject::update() {
    if (animation) {
        animation->update();
    }
}

std::vector<std::shared_ptr<RenderConfigInterface>> Textured2dLayerObject::getRenderConfig() { return {renderConfig}; }

void Textured2dLayerObject::setAlpha(float alpha) {
    // setAlpha only works for AlphaShaders
    // use setStyle for RasterShader
    assert(shader != nullptr);
    if (shader) {
        shader->updateAlpha(alpha);
    }
    mapInterface->invalidate();
}

void Textured2dLayerObject::setStyle(const RasterShaderStyle &style) {
    // setStyle only works for RasterShaders
    // use setAlpha for AlphaShader
    assert(rasterShader != nullptr);
    if (rasterShader) {
        rasterShader->setStyle(style);
    }
    mapInterface->invalidate();
}

std::shared_ptr<Quad2dInterface> Textured2dLayerObject::getQuadObject() { return quad; }

void Textured2dLayerObject::setSubdivisionFactor(int32_t factor) {
    if (texturedPolygon) {
        texturedPolygon->setSubdivisionFactor(factor);
    } else if (quad) {
        quad->setSubdivisionFactor(factor);
    }
}

void Textured2dLayerObject::setMinMagFilter(TextureFilterType filterType) {
    if (texturedPolygon) {
        texturedPolygon->setMinMagFilter(filterType);
    } else if (quad) {
        quad->setMinMagFilter(filterType);
    }
}

void Textured2dLayerObject::loadTexture(const std::shared_ptr<RenderingContextInterface> &context,
                                        const std::shared_ptr<TextureHolderInterface> &textureHolder) {
    if (texturedPolygon) {
        texturedPolygon->loadTexture(context, textureHolder);
    } else if (quad) {
        quad->loadTexture(context, textureHolder);
    }
}

void Textured2dLayerObject::loadDualTexture(const std::shared_ptr<RenderingContextInterface> &context,
                                            const std::shared_ptr<TextureHolderInterface> &textureHolder,
                                            const std::shared_ptr<TextureHolderInterface> &elevationHolder) {
    if (texturedPolygon) {
        texturedPolygon->loadDualTexture(context, textureHolder, elevationHolder);
    } else if (quad) {
        quad->loadDualTexture(context, textureHolder, elevationHolder);
    }
}

void Textured2dLayerObject::loadTextures(const std::shared_ptr<RenderingContextInterface> &context,
                                         const std::shared_ptr<TextureHolderInterface> &textureHolder,
                                         const std::shared_ptr<TextureHolderInterface> &lookupHolder,
                                         const std::shared_ptr<TextureHolderInterface> &elevationHolder) {
    if (texturedPolygon) {
        // Textured polygons have no dedicated triple-texture binding; the lookup sprite is their secondary texture.
        texturedPolygon->loadDualTexture(context, textureHolder, lookupHolder);
    } else if (quad) {
        quad->loadTextures(context, textureHolder, lookupHolder, elevationHolder);
    }
}

void Textured2dLayerObject::removeTexture() {
    if (texturedPolygon) {
        texturedPolygon->removeTexture();
    } else if (quad) {
        quad->removeTexture();
    }
}

std::shared_ptr<GraphicsObjectInterface> Textured2dLayerObject::getGraphicsObject() { return graphicsObject; }

std::shared_ptr<RenderObjectInterface> Textured2dLayerObject::getRenderObject() {
    return renderObject;
}

void Textured2dLayerObject::beginAlphaAnimation(double startAlpha, double targetAlpha, int64_t duration) {
    assert(shader != nullptr);
    std::weak_ptr<Textured2dLayerObject> weakSelf = weak_from_this();
    animation = std::make_shared<DoubleAnimation>(
            duration, startAlpha, targetAlpha, InterpolatorFunction::EaseIn,
            [weakSelf](double alpha) {
                if (auto selfPtr = weakSelf.lock()) {
                    selfPtr->setAlpha(alpha);
                }
            },
            [weakSelf, targetAlpha] {
                if (auto selfPtr = weakSelf.lock()) {
                    selfPtr->setAlpha(targetAlpha);
                    selfPtr->animation = nullptr;
                }
            });
    animation->start();
    mapInterface->invalidate();
}

void Textured2dLayerObject::beginStyleAnimation(RasterShaderStyle start, RasterShaderStyle target, int64_t duration) {
    assert(rasterShader != nullptr);
    std::weak_ptr<Textured2dLayerObject> weakSelf = weak_from_this();
    animation = std::make_shared<RasterStyleAnimation>(
            duration, start, target, InterpolatorFunction::EaseIn,
            [weakSelf](RasterShaderStyle style) {
                if (auto selfPtr = weakSelf.lock()) {
                    selfPtr->setStyle(style);
                }
            },
            [weakSelf, target] {
                if (auto selfPtr = weakSelf.lock()) {
                    selfPtr->setStyle(target);
                    selfPtr->animation = nullptr;
                }
            });
    animation->start();
    mapInterface->invalidate();
}

std::shared_ptr<ShaderProgramInterface> Textured2dLayerObject::getShader() {
    if (rasterShader) {
        return rasterShader->asShaderProgramInterface();
    }
    if (shader) {
        return shader->asShaderProgramInterface();
    }
    return nullptr;
}
