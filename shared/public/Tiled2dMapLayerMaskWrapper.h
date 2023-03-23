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

#include "PolygonMaskObject.h"

class Tiled2dMapLayerMaskWrapper {
public:
    Tiled2dMapLayerMaskWrapper(std::shared_ptr<PolygonMaskObject> maskObject, size_t polygonHash): maskObject(maskObject), polygonHash(polygonHash), graphicsObject(maskObject->getPolygonObject()->asGraphicsObject()), graphicsMaskObject(maskObject->getPolygonObject()->asMaskingObject()) {}

    Tiled2dMapLayerMaskWrapper(): maskObject(nullptr), graphicsObject(nullptr), graphicsMaskObject(nullptr), polygonHash(0) {}

    size_t getPolygonHash() const { return polygonHash; }
    std::shared_ptr<GraphicsObjectInterface> getGraphicsObject() const { return graphicsObject; }
    std::shared_ptr<MaskingObjectInterface> getGraphicsMaskObject() const { return graphicsMaskObject; }

private:
    std::shared_ptr<PolygonMaskObject> maskObject;
    std::shared_ptr<GraphicsObjectInterface> graphicsObject;
    std::shared_ptr<MaskingObjectInterface> graphicsMaskObject;
    size_t polygonHash;
};
