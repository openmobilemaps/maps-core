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

#include "Tiled2dMapVectorSymbolObject.h"

class SymbolObjectCollisionWrapper {
public:
    std::shared_ptr<Tiled2dMapVectorSymbolObject> symbolObject;
    double symbolSortKey;
    size_t symbolTileIndex;
    bool isColliding;

    SymbolObjectCollisionWrapper(const std::shared_ptr<Tiled2dMapVectorSymbolObject> &object)
            : symbolObject(object), symbolSortKey(object->symbolSortKey), symbolTileIndex(object->symbolTileIndex),
              isColliding(object->animationCoordinator->isColliding()) {};

    SymbolObjectCollisionWrapper(const SymbolObjectCollisionWrapper& other) noexcept
            : symbolObject(std::move(other.symbolObject)),
              symbolSortKey(other.symbolSortKey),
              symbolTileIndex(other.symbolTileIndex),
              isColliding(other.isColliding) {}

    SymbolObjectCollisionWrapper(SymbolObjectCollisionWrapper &other) = delete;

    SymbolObjectCollisionWrapper& operator=(SymbolObjectCollisionWrapper&& other) noexcept {
        if (this != &other) {
            symbolObject = std::move(other.symbolObject);
            symbolSortKey = other.symbolSortKey;
            symbolTileIndex = other.symbolTileIndex;
            isColliding = other.isColliding;
        }
        return *this;
    }

    bool operator<(const SymbolObjectCollisionWrapper &o) const {
        if (isColliding != o.isColliding) {
            return isColliding;
        }
        if (symbolObject->smallestVisibleZoom != o.symbolObject->smallestVisibleZoom) {
            return symbolObject->smallestVisibleZoom > o.symbolObject->smallestVisibleZoom;
        }
        if (symbolSortKey == o.symbolSortKey) {
            return symbolTileIndex > o.symbolTileIndex;
        }
        return symbolSortKey > o.symbolSortKey;
    }
};
