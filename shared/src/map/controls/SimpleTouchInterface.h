//
// Created by Christoph Maurhofer on 04.02.2021.
//

#pragma once

#include "TouchInterface.h"

class SimpleTouchInterface : public TouchInterface {
public:
    virtual bool onClickUnconfirmed(const ::Vec2F &posScreen) override {
        return false;
    };

    virtual bool onClickConfirmed(const ::Vec2F &posScreen) override {
        return false;
    };

    virtual bool onDoubleClick(const ::Vec2F &posScreen) override {
        return false;
    };

    virtual bool onLongPress(const ::Vec2F &posScreen) override {
        return false;
    };

    virtual bool onMove(const ::Vec2F &deltaScreen, bool confirmed, bool doubleClick) override {
        return false;
    };

    virtual bool onMoveComplete() override {
        return false;
    };

    virtual bool onTwoFingerClick(const ::Vec2F &posScreen1, const ::Vec2F &posScreen2) override {
        return false;
    };

    virtual bool onTwoFingerMove(const std::vector<::Vec2F> &posScreenOld, const std::vector<::Vec2F> &posScreenNew) override {
        return false;
    };

    virtual void clearTouch() override {};
};


