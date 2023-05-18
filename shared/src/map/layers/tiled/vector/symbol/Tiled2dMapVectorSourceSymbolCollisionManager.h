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

#include "Actor.h"
#include "Tiled2dMapVectorSourceSymbolDataManager.h"

class Tiled2dMapVectorSourceSymbolCollisionManager: public ActorObject {

public:
    Tiled2dMapVectorSourceSymbolCollisionManager(const std::unordered_map<std::string, WeakActor<Tiled2dMapVectorSourceSymbolDataManager>> &symbolSourceDataManagers,
                                                 std::shared_ptr<VectorMapDescription> mapDescription): symbolSourceDataManagers(symbolSourceDataManagers), mapDescription(mapDescription)  {};

    void update();
    
private:
    void collisionDetection();

    std::unordered_map<std::string, WeakActor<Tiled2dMapVectorSourceSymbolDataManager>> symbolSourceDataManagers;
    std::shared_ptr<VectorMapDescription> mapDescription;
};
