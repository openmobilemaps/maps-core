#pragma once

#include "TouchHandlerInterface.h"
#include "SchedulerInterface.h"
#include <vector>
#include "Vec2F.h"

class DefaultTouchHandler: public TouchHandlerInterface {

public:
    DefaultTouchHandler(const std::shared_ptr<SchedulerInterface> SchedulerInterface, float density);

    virtual void onTouchEvent(const TouchEvent & touchEvent);

    virtual void addListener(const std::shared_ptr<TouchInterface> & listener);

    virtual void removeListener(const std::shared_ptr<TouchInterface> & listener);

private:

    enum TouchHandlingState {
        IDLE,
        ONE_FINGER_DOWN,
        ONE_FINGER_MOVING,
        ONE_FINGER_UP_AFTER_CLICK,
        ONE_FINGER_DOUBLE_CLICK_DOWN,
        ONE_FINGER_DOUBLE_CLICK_MOVE,
        TWO_FINGER_DOWN,
        TWO_FINGER_MOVING
    };


    void handleTouchDown(Vec2F position);

    void handleMove(Vec2F delta);

    void handleTouchUp();

    void handleTwoFingerDown();

    void handleTwoFingerMove(std::tuple<Vec2F, Vec2F> oldpointer, std::tuple<Vec2F, Vec2F> pointer);

    void handleTwoFingerUp(std::tuple<Vec2F, Vec2F> doubleTouchPointer);

    void handleMoreThanTwoFingers();

    void checkState();

    int32_t TWO_FINGER_TOUCH_TIMEOUT = 100;
    int32_t DOUBLE_TAP_TIMEOUT = 300;
    int32_t LONG_PRESS_TIMEOUT = 500;
    int32_t CLICK_DISTANCE_MM = 3;

    float density;
    float clickDistancePx;

    std::vector<std::shared_ptr<TouchInterface>> listeners;
    
    const std::shared_ptr<SchedulerInterface> scheduler;

    TouchHandlingState state;
    long long stateTime;

    Vec2F touchPosition;
    Vec2F touchStartPosition;

    std::tuple<Vec2F, Vec2F> pointer;
    std::tuple<Vec2F, Vec2F> oldPointer;
};
