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

#include "VectorLayerDescription.h"
#include "Color.h"
#include "ColorUtil.h"

class BackgroundVectorStyle {
public:
    BackgroundVectorStyle(std::shared_ptr<Value> color): color(color) {}

    std::unordered_set<std::string> getUsedKeys() {

        std::unordered_set<std::string> usedKeys;
        std::vector<std::shared_ptr<Value>> values = {
            color
        };

        for (auto const &value: values) {
            if (!value) continue;
            auto const setKeys = value->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }

        return usedKeys;
    };

    Color getColor(const EvaluationContext &context){
        static const Color defaultValue = ColorUtil::c(0, 0, 0, 1.0);
        return color ? color->evaluateOr(context, defaultValue) : defaultValue;
    }

private:
    std::shared_ptr<Value> color;
};

class BackgroundVectorLayerDescription: public VectorLayerDescription {
public:
    VectorLayerType getType() override { return VectorLayerType::background; };
    BackgroundVectorStyle style;

    BackgroundVectorLayerDescription(std::string identifier,
                                     BackgroundVectorStyle style,
                                     std::optional<int32_t> renderPassIndex):
    VectorLayerDescription(identifier, "", "", 0, 0, nullptr, renderPassIndex),
    style(style) {};

    virtual std::unordered_set<std::string> getUsedKeys() override {
        std::unordered_set<std::string> usedKeys;

        auto parentKeys = VectorLayerDescription::getUsedKeys();
        usedKeys.insert(parentKeys.begin(), parentKeys.end());

        auto styleKeys = style.getUsedKeys();
        usedKeys.insert(styleKeys.begin(), styleKeys.end());

        return usedKeys;
    };
};
