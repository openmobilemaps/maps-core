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

#include "Tiled2dMapVectorSourceDataManager.h"
#include "VectorMapSourceDescription.h"
#include "TouchInterface.h"
#include <unordered_map>

class Tiled2dMapVectorInteractionManager : public TouchInterface {
public:
    Tiled2dMapVectorInteractionManager(const std::unordered_map<std::string, std::vector<WeakActor<Tiled2dMapVectorSourceDataManager>>> &sourceDataManagers,
                                       const std::shared_ptr<VectorMapDescription> &mapDescription);
    Tiled2dMapVectorInteractionManager() = default;

    bool onTouchDown(const Vec2F &posScreen) override;

    bool onClickUnconfirmed(const Vec2F &posScreen) override;

    bool onClickConfirmed(const Vec2F &posScreen) override;

    bool onDoubleClick(const Vec2F &posScreen) override;

    bool onLongPress(const Vec2F &posScreen) override;

    bool onMove(const Vec2F &deltaScreen, bool confirmed, bool doubleClick) override;

    bool onMoveComplete() override;

    bool onTwoFingerClick(const Vec2F &posScreen1, const Vec2F &posScreen2) override;

    bool onTwoFingerMove(const std::vector<::Vec2F> &posScreenOld, const std::vector<::Vec2F> &posScreenNew) override;

    bool onTwoFingerMoveComplete() override;

    void clearTouch() override;

    void performClick(const Coord &coord);

private:
    template<typename F>
    inline bool callInReverseOrder(F&& managerLambda);

    std::unordered_map<std::string, std::vector<WeakActor<Tiled2dMapVectorSourceDataManager>>> sourceDataManagersMap;
    std::shared_ptr<VectorMapDescription> mapDescription;
};
