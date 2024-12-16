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

#include "SchedulerInterface.h"
#include "TouchHandlerInterface.h"
#include "Vec2F.h"
#include <list>
#include <map>
#include <mutex>

class DefaultTouchHandler : public TouchHandlerInterface {

public:
    DefaultTouchHandler(const std::shared_ptr<SchedulerInterface> SchedulerInterface, float density);

    virtual void onTouchEvent(const TouchEvent &touchEvent) override;

    virtual void addListener(const std::shared_ptr<TouchInterface> &listener) override;

    virtual void removeListener(const std::shared_ptr<TouchInterface> &listener) override;

    virtual void insertListener(const std::shared_ptr<TouchInterface> &listener, int32_t index) override;

private:
    struct ListenerEntry {
        int32_t index;
        std::shared_ptr<TouchInterface> listener;
    };

    enum TouchHandlingState {
        IDLE,
        ONE_FINGER_DOWN,
        ONE_FINGER_MOVING,
        ONE_FINGER_UP_AFTER_CLICK,
        ONE_FINGER_DOUBLE_CLICK_DOWN,
        ONE_FINGER_DOUBLE_CLICK_MOVE,
        TWO_FINGER_DOWN,
        TWO_FINGER_MOVING,
        ONE_FINGER_AFTER_TWO
    };

    void handleTouchDown(Vec2F position);

    void handleMove(Vec2F delta);

    void handleTouchUp();

    void handleTouchCancel();

    void handleTwoFingerDown();

    void handleTwoFingerMove(std::tuple<Vec2F, Vec2F> oldpointer, std::tuple<Vec2F, Vec2F> pointer);

    void handleTwoFingerUp(std::tuple<Vec2F, Vec2F> doubleTouchPointer);

    void handleMoreThanTwoFingers();

    std::recursive_mutex stateMutex;

    void checkState();

    int32_t TWO_FINGER_TOUCH_TIMEOUT = 100;
    int32_t DOUBLE_TAP_TIMEOUT = 300;
    int32_t LONG_PRESS_TIMEOUT = 500;
    int32_t CLICK_DISTANCE_MM = 3;

    float density;
    float clickDistancePx;

    std::recursive_mutex listenerMutex;
    std::list<ListenerEntry> listeners; // ordered by the decreasing index

    const std::weak_ptr<SchedulerInterface> scheduler;

    TouchHandlingState state;
    long long stateTime;

    Vec2F touchPosition;
    Vec2F touchStartPosition;

    std::tuple<Vec2F, Vec2F> pointer;
    std::tuple<Vec2F, Vec2F> oldPointer;
};
