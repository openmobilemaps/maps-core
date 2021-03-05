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

#include "TouchInterface.h"

class SimpleTouchInterface : public TouchInterface {
  public:
    virtual bool onTouchDown(const ::Vec2F &posScreen) override { return false; };

    virtual bool onClickUnconfirmed(const ::Vec2F &posScreen) override { return false; };

    virtual bool onClickConfirmed(const ::Vec2F &posScreen) override { return false; };

    virtual bool onDoubleClick(const ::Vec2F &posScreen) override { return false; };

    virtual bool onLongPress(const ::Vec2F &posScreen) override { return false; };

    virtual bool onMove(const ::Vec2F &deltaScreen, bool confirmed, bool doubleClick) override { return false; };

    virtual bool onMoveComplete() override { return false; };

    virtual bool onTwoFingerClick(const ::Vec2F &posScreen1, const ::Vec2F &posScreen2) override { return false; };

    virtual bool onTwoFingerMove(const std::vector<::Vec2F> &posScreenOld, const std::vector<::Vec2F> &posScreenNew) override {
        return false;
    };

    virtual void clearTouch() override{};
};
