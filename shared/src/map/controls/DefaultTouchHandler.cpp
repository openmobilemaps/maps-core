/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "DefaultTouchHandler.h"
#include "DateHelper.h"
#include "LambdaTask.h"
#include "Logger.h"
#include "TouchEvent.h"
#include "TouchInterface.h"
#include <cmath>

DefaultTouchHandler::DefaultTouchHandler(std::shared_ptr<SchedulerInterface> scheduler, float density)
    : scheduler(scheduler)
    , density(density)
    , clickDistancePx(CLICK_DISTANCE_MM * density / 25.4)
    , state(IDLE)
    , stateTime(0)
    , touchPosition(0, 0)
    , touchStartPosition(0, 0)
    , pointer(Vec2F(0, 0), Vec2F(0, 0))
    , oldPointer(Vec2F(0, 0), Vec2F(0, 0)) {}

void DefaultTouchHandler::addListener(const std::shared_ptr<TouchInterface> &listener) {
    std::lock_guard<std::recursive_mutex> lock(listenerMutex);
    if (listeners.empty()) {
        listeners.push_back({0, listener});
    } else {
        auto newIndex = listeners.begin()->index + 1;
        listeners.push_front({newIndex, listener});
    }
}

void DefaultTouchHandler::insertListener(const std::shared_ptr<TouchInterface> &listener, int32_t index) {
    std::lock_guard<std::recursive_mutex> lock(listenerMutex);
    for (auto iter = listeners.begin(); iter != listeners.end(); iter++) {
        if (iter->index <= index) {
            listeners.insert(iter, {index, listener});
            break;
        }
    }
}

void DefaultTouchHandler::removeListener(const std::shared_ptr<TouchInterface> &listener) {
    std::lock_guard<std::recursive_mutex> lock(listenerMutex);
    for (auto it = listeners.begin(); it != listeners.end();) {
        if (it->listener == listener) {
            listeners.erase(it++);
        } else {
            ++it;
        }
    }
}

void DefaultTouchHandler::onTouchEvent(const TouchEvent &touchEvent) {

    std::lock_guard<std::recursive_mutex> lock(stateMutex);

    if (touchEvent.pointers.size() == 1) {

        switch (touchEvent.touchAction) {
        case TouchAction::DOWN: {
            touchPosition = touchEvent.pointers[0];
            touchStartPosition = touchPosition;
            handleTouchDown(touchPosition);
            break;
        }

        case TouchAction::MOVE: {
            if (state != ONE_FINGER_DOWN && state != ONE_FINGER_DOUBLE_CLICK_DOWN && state != ONE_FINGER_MOVING &&
                state != ONE_FINGER_DOUBLE_CLICK_MOVE) {
                touchPosition = touchEvent.pointers[0];
            }

            Vec2F delta = Vec2F(touchEvent.pointers[0].x - touchPosition.x, touchEvent.pointers[0].y - touchPosition.y);

            touchPosition = touchEvent.pointers[0];

            handleMove(delta);
            break;
        }

        case TouchAction::UP: {
            handleTouchUp();
            break;
        }

        case TouchAction::CANCEL: {
            handleTouchCancel();
            break;
        }
        }

    } else if (touchEvent.pointers.size() == 2) {

        switch (touchEvent.touchAction) {
        case TouchAction::DOWN: {
            pointer = {Vec2F(0, 0), Vec2F(0, 0)};
            oldPointer = {touchEvent.pointers[0], touchEvent.pointers[1]};
            handleTwoFingerDown();
            break;
        }

        case TouchAction::MOVE: {
            oldPointer = pointer;
            pointer = {touchEvent.pointers[0], touchEvent.pointers[1]};

            if (std::get<0>(oldPointer).x != 0 || std::get<0>(oldPointer).y != 0 || std::get<1>(oldPointer).x != 0 ||
                std::get<1>(oldPointer).y != 0) {
                handleTwoFingerMove(oldPointer, pointer);
            }

            oldPointer = pointer;
            break;
        }

        case TouchAction::UP: {
            handleTwoFingerUp(oldPointer);
            break;
        }

        case TouchAction::CANCEL: {
            handleTouchCancel();
            break;
        }
        }

    } else {
        pointer = {Vec2F(0, 0), Vec2F(0, 0)};
        oldPointer = {Vec2F(0, 0), Vec2F(0, 0)};
        handleMoreThanTwoFingers();
    }
}

double distance(std::vector<float> &pointer) {
    return sqrt((pointer[0] - pointer[2]) * (pointer[0] - pointer[2]) + (pointer[1] - pointer[3]) * (pointer[1] - pointer[3]));
}

bool multiTouchMoved(std::tuple<Vec2F, Vec2F> &pointer1, std::tuple<Vec2F, Vec2F> &pointer2, float value) {
    float diffA =
        (float)sqrt((std::get<0>(pointer1).x - std::get<0>(pointer2).x) * (std::get<0>(pointer1).x - std::get<0>(pointer2).x) +
                    (std::get<0>(pointer1).y - std::get<0>(pointer1).y) * (std::get<0>(pointer1).y - std::get<0>(pointer1).y));
    float diffB =
        (float)sqrt((std::get<1>(pointer1).x - std::get<1>(pointer1).x) * (std::get<1>(pointer1).x - std::get<1>(pointer1).x) +
                    (std::get<1>(pointer1).y - std::get<1>(pointer1).y) * (std::get<1>(pointer1).y - std::get<1>(pointer1).y));
    return (diffA > value || diffB > value);
}

void DefaultTouchHandler::handleTouchDown(Vec2F position) {

    std::lock_guard<std::recursive_mutex> lock(stateMutex);

    if (state == ONE_FINGER_UP_AFTER_CLICK && stateTime >= DateHelper::currentTimeMillis() - DOUBLE_TAP_TIMEOUT) {
        state = ONE_FINGER_DOUBLE_CLICK_DOWN;
    } else {
#ifdef ENABLE_TOUCH_LOGGING
        LogDebug <<= "TouchHandler: is touching down (one finger)";
#endif
        state = ONE_FINGER_DOWN;
    }
    stateTime = DateHelper::currentTimeMillis();
    auto strongScheduler = scheduler.lock();
    if (strongScheduler) {
        strongScheduler->addTask(std::make_shared<LambdaTask>(
                                                              TaskConfig("LongPressTask", LONG_PRESS_TIMEOUT, TaskPriority::NORMAL, ExecutionEnvironment::COMPUTATION),
                                                              [=] { checkState(); }));
    }
    {
        std::lock_guard<std::recursive_mutex> lock(listenerMutex);
        for (auto &[index, listener]: listeners) {
            if (listener->onTouchDown(position)) {
                break;
            }
        }
    }
}

void DefaultTouchHandler::handleMove(Vec2F delta) {

    std::lock_guard<std::recursive_mutex> lock(stateMutex);

#ifdef ENABLE_TOUCH_LOGGING
    LogDebug <<= "TouchHandler: handle move";
#endif
    std::vector<float> diffPointer = {touchStartPosition.x, touchStartPosition.y, touchPosition.x, touchPosition.y};
    if (distance(diffPointer) > clickDistancePx) {
#ifdef ENABLE_TOUCH_LOGGING
        LogDebug <<= "TouchHandler: moved large distance";
#endif
        if (state == ONE_FINGER_DOUBLE_CLICK_DOWN || state == ONE_FINGER_DOUBLE_CLICK_MOVE) {
            state = ONE_FINGER_DOUBLE_CLICK_MOVE;
        }
        else if (state == TWO_FINGER_DOWN) {
            state = TWO_FINGER_MOVING;
        }
        else if (state == ONE_FINGER_AFTER_TWO) {
            // no action
        }
        else {
#ifdef ENABLE_TOUCH_LOGGING
            LogDebug <<= "TouchHandler: is moving now";
#endif
            state = ONE_FINGER_MOVING;
        }
        stateTime = DateHelper::currentTimeMillis();
    }
    {
        std::lock_guard<std::recursive_mutex> lock(listenerMutex);
        for (auto &[index, listener]: listeners) {
            if (listener->onMove(delta,
                                 state == ONE_FINGER_MOVING,             // confirmed
                                 state == ONE_FINGER_DOUBLE_CLICK_MOVE)) // double click move
            {
                break;
            }
        }
    }
}

void DefaultTouchHandler::handleTouchUp() {

    std::lock_guard<std::recursive_mutex> lock(stateMutex);

    if (state == ONE_FINGER_DOUBLE_CLICK_MOVE) {
#ifdef ENABLE_TOUCH_LOGGING
        LogDebug <<= "TouchHandler: double click move ended";
#endif
        {
            std::lock_guard<std::recursive_mutex> lock(listenerMutex);
            for (auto &[index, listener]: listeners) {
                if (listener->onOneFingerDoubleClickMoveComplete()) {
                    break;
                }
            }
        }
        state = IDLE;
    } else if (state == ONE_FINGER_DOUBLE_CLICK_DOWN) {
#ifdef ENABLE_TOUCH_LOGGING
        LogDebug <<= "TouchHandler: double click detected";
#endif
        {
            std::lock_guard<std::recursive_mutex> lock(listenerMutex);
            for (auto &[index, listener]: listeners) {
                if (listener->onDoubleClick(touchPosition)) {
                    break;
                }
            }
        }
        state = IDLE;
    } else if (state == ONE_FINGER_DOWN) {
#ifdef ENABLE_TOUCH_LOGGING
        LogDebug <<= "TouchHandler: unconfirmed click detected";
#endif
        bool clickHandled = false;
        {
            std::lock_guard<std::recursive_mutex> lock(listenerMutex);
            for (auto &[index, listener]: listeners) {
                if (listener->onClickUnconfirmed(touchPosition)) {
                    clickHandled = true;
                    break;
                }
            }
        }
        if (clickHandled) {
            state = IDLE;
        } else {
            state = ONE_FINGER_UP_AFTER_CLICK;
            auto strongScheduler = scheduler.lock();
            if (strongScheduler) {
                strongScheduler->addTask(std::make_shared<LambdaTask>(
                                                                      TaskConfig("DoubleTapTask", DOUBLE_TAP_TIMEOUT, TaskPriority::NORMAL, ExecutionEnvironment::COMPUTATION),
                                                                      [=] { checkState(); }));
            }
        }

    } else if (state == TWO_FINGER_DOWN && stateTime >= DateHelper::currentTimeMillis() - TWO_FINGER_TOUCH_TIMEOUT) {
#ifdef ENABLE_TOUCH_LOGGING
        LogDebug <<= "TouchHandler: Two finger click detected";
#endif
        {
            std::lock_guard<std::recursive_mutex> lock(listenerMutex);
            for (auto &[index, listener]: listeners) {
                if (listener->onTwoFingerClick(std::get<0>(oldPointer), std::get<1>(oldPointer))) {
                    break;
                }
            }
        }

    } else {
        if (state == ONE_FINGER_MOVING) {
            {
                std::lock_guard<std::recursive_mutex> lock(listenerMutex);
                for (auto &[index, listener]: listeners) {
                    if (listener->onMoveComplete()) {
                        break;
                    }
                }
            }
        }

        if (state == TWO_FINGER_DOWN || state == TWO_FINGER_MOVING) {
            state = ONE_FINGER_AFTER_TWO;
            stateTime = DateHelper::currentTimeMillis();
            auto strongScheduler = scheduler.lock();
            if (strongScheduler) {
                strongScheduler->addTask(std::make_shared<LambdaTask>(
                                                                      TaskConfig("OneFingerAfterTwoTask", TWO_FINGER_TOUCH_TIMEOUT, TaskPriority::NORMAL, ExecutionEnvironment::COMPUTATION),
                                                                      [=] { checkState(); }));
            }
        }
        else {
            state = IDLE;
        }
    }
    {
        std::lock_guard<std::recursive_mutex> lock(listenerMutex);
        for (auto &[index, listener]: listeners) {
            listener->clearTouch();
        }
    }
    stateTime = DateHelper::currentTimeMillis();
}

void DefaultTouchHandler::handleTouchCancel() {

    std::lock_guard<std::recursive_mutex> lock(stateMutex);

    state = IDLE;
    {
        std::lock_guard<std::recursive_mutex> lock(listenerMutex);
        for (auto &[index, listener]: listeners) {
            listener->clearTouch();
        }
    }
    stateTime = DateHelper::currentTimeMillis();
}

void DefaultTouchHandler::handleTwoFingerDown() {

    std::lock_guard<std::recursive_mutex> lock(stateMutex);

    if (state == ONE_FINGER_MOVING) {
        {
            std::lock_guard<std::recursive_mutex> lock(listenerMutex);
            for (auto &[index, listener]: listeners) {
                if (listener->onMoveComplete()) {
                    break;
                }
            }
        }
    }
    state = TWO_FINGER_DOWN;
    stateTime = DateHelper::currentTimeMillis();
    auto strongScheduler = scheduler.lock();
    if (strongScheduler) {
        strongScheduler->addTask(std::make_shared<LambdaTask>(
                                                              TaskConfig("LongPressTask", LONG_PRESS_TIMEOUT, TaskPriority::NORMAL, ExecutionEnvironment::COMPUTATION),
                                                              [=] { checkState(); }));
    }
    {
        std::lock_guard<std::recursive_mutex> lock(listenerMutex);
        for (auto &[index, listener]: listeners) {
            listener->clearTouch();
        }
    }
}

void DefaultTouchHandler::handleTwoFingerMove(std::tuple<Vec2F, Vec2F> oldPointer, std::tuple<Vec2F, Vec2F> newpointer) {

    std::lock_guard<std::recursive_mutex> lock(stateMutex);

    if (state == ONE_FINGER_MOVING) {
        {
            std::lock_guard<std::recursive_mutex> lock(listenerMutex);
            for (auto &[index, listener]: listeners) {
                if (listener->onMoveComplete()) {
                    break;
                }
            }
        }
    }
    if (multiTouchMoved(oldPointer, newpointer, clickDistancePx)) {
        state = TWO_FINGER_MOVING;
        stateTime = DateHelper::currentTimeMillis();
    }
    {
        std::lock_guard<std::recursive_mutex> lock(listenerMutex);
        for (auto &[index, listener]: listeners) {
            if (listener->onTwoFingerMove({std::get<0>(oldPointer), std::get<1>(oldPointer)},
                                          {std::get<0>(newpointer), std::get<1>(newpointer)})) {
                break;
            }
        }
    }
}

void DefaultTouchHandler::handleTwoFingerUp(std::tuple<Vec2F, Vec2F> doubleTouchPointer) {

    std::lock_guard<std::recursive_mutex> lock(stateMutex);

    if (state != TWO_FINGER_DOWN) {
        state = ONE_FINGER_AFTER_TWO;
        stateTime = DateHelper::currentTimeMillis();
        auto strongScheduler = scheduler.lock();
        if (strongScheduler) {
            strongScheduler->addTask(std::make_shared<LambdaTask>(
                                                                  TaskConfig("OneFingerAfterTwoTask", TWO_FINGER_TOUCH_TIMEOUT, TaskPriority::NORMAL, ExecutionEnvironment::COMPUTATION),
                                                                  [=] { checkState(); }));
        }
        {
            std::lock_guard<std::recursive_mutex> lock(listenerMutex);
            for (auto &[index, listener]: listeners) {
                if (listener->onTwoFingerMoveComplete()) {
                    break;
                }
            }
        }
    }
}

void DefaultTouchHandler::handleMoreThanTwoFingers() {

    std::lock_guard<std::recursive_mutex> lock(stateMutex);

    if (state == ONE_FINGER_MOVING) {
        {
            std::lock_guard<std::recursive_mutex> lock(listenerMutex);
            for (auto &[index, listener]: listeners) {
                if (listener->onMoveComplete()) {
                    break;
                }
            }
        }
    }
    state = IDLE;
    stateTime = DateHelper::currentTimeMillis();
    {
        std::lock_guard<std::recursive_mutex> lock(listenerMutex);
        for (auto &[index, listener]: listeners) {
            listener->clearTouch();
        }
    }
}

void DefaultTouchHandler::checkState() {

    bool onClickConfirmed = false;
    bool onLongPress = false;

    { // begin lock stateMutex
        std::lock_guard<std::recursive_mutex> lock(stateMutex);

        if (state == ONE_FINGER_UP_AFTER_CLICK && stateTime <= DateHelper::currentTimeMillis() - DOUBLE_TAP_TIMEOUT) {
#ifdef ENABLE_TOUCH_LOGGING
            LogDebug <<= "TouchHandler: confirmed click detected";
#endif
            onClickConfirmed = true;
            state = IDLE;
            stateTime = DateHelper::currentTimeMillis();
        } else if (state == ONE_FINGER_DOWN && stateTime <= DateHelper::currentTimeMillis() - LONG_PRESS_TIMEOUT) {
#ifdef ENABLE_TOUCH_LOGGING
            LogDebug <<= "TouchHandler: long press detected";
#endif
            onLongPress = true;
            state = ONE_FINGER_MOVING; // prevents further single click and allows to transition from long press to moving
            stateTime = DateHelper::currentTimeMillis();
        } else if (state == TWO_FINGER_DOWN && stateTime <= DateHelper::currentTimeMillis() - LONG_PRESS_TIMEOUT) {
            state = TWO_FINGER_MOVING;
            stateTime = DateHelper::currentTimeMillis();
        }
        else if (state == ONE_FINGER_AFTER_TWO && stateTime <= DateHelper::currentTimeMillis() - TWO_FINGER_TOUCH_TIMEOUT) {
            state = IDLE;
            stateTime = DateHelper::currentTimeMillis();
        }
    } // end lock stateMutex

    if (onClickConfirmed) {
        std::lock_guard<std::recursive_mutex> lock(listenerMutex);
        for (auto &[index, listener]: listeners) {
            if (listener->onClickConfirmed(touchPosition)) {
                break;
            }
        }
    }
    else if (onLongPress) {
        std::lock_guard<std::recursive_mutex> lock(listenerMutex);
        for (auto &[index, listener]: listeners) {
            if (listener->onLongPress(touchPosition)) {
                break;
            }
        }
    }
}
