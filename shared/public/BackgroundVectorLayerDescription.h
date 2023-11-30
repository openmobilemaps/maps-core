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
    BackgroundVectorStyle(std::shared_ptr<Value> backgroundColor,
                          std::shared_ptr<Value> backgroundPattern,
                          std::shared_ptr<Value> blendMode): backgroundColor(backgroundColor), backgroundPattern(backgroundPattern), blendMode(blendMode) {}

    UsedKeysCollection getUsedKeys() const {

        UsedKeysCollection usedKeys;
        std::vector<std::shared_ptr<Value>> values = {
            backgroundColor, backgroundPattern, blendMode
        };

        for (auto const &value: values) {
            if (!value) continue;
            auto const keys = value->getUsedKeys();
            usedKeys.includeOther(keys);
        }

        return usedKeys;
    };

    BlendMode getBlendMode(const EvaluationContext &context) const {
        static const BlendMode defaultValue = BlendMode::NORMAL;
        return blendMode ? blendMode->evaluateOr(context, defaultValue) : defaultValue;
    }

    Color getColor(const EvaluationContext &context) {
        static const Color defaultValue = ColorUtil::c(0, 0, 0, 1.0);
        return backgroundColor ? backgroundColor->evaluateOr(context, defaultValue) : defaultValue;
    }

    std::string getPattern(const EvaluationContext &context) {
         static const std::string defaultValue = "";
         return backgroundPattern ? backgroundPattern->evaluateOr(context, defaultValue) : defaultValue;
     }

    std::shared_ptr<Value> backgroundColor;
    std::shared_ptr<Value> backgroundPattern;
    std::shared_ptr<Value> blendMode;
};

class BackgroundVectorLayerDescription: public VectorLayerDescription {
public:
    VectorLayerType getType() override { return VectorLayerType::background; };
    BackgroundVectorStyle style;

    BackgroundVectorLayerDescription(std::string identifier,
                                     BackgroundVectorStyle style,
                                     std::optional<int32_t> renderPassIndex,
                                     std::shared_ptr<Value> interactable):
    VectorLayerDescription(identifier, "", "", 0, 0, nullptr, renderPassIndex, interactable),
    style(style) {};

    std::unique_ptr<VectorLayerDescription> clone() override {
        return std::make_unique<BackgroundVectorLayerDescription>(identifier, style, renderPassIndex,
                                                                  interactable ? interactable->clone() : nullptr);
    }

    virtual UsedKeysCollection getUsedKeys() const override {
        UsedKeysCollection usedKeys;

        auto parentKeys = VectorLayerDescription::getUsedKeys();
        usedKeys.includeOther(parentKeys);

        auto styleKeys = style.getUsedKeys();
        usedKeys.includeOther(styleKeys);

        return usedKeys;
    };
};
