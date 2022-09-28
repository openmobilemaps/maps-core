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
#include "Font.h"
#include "Vec2F.h"
#include "Value.h"
#include "TextJustify.h"
#include "ColorUtil.h"


class SymbolVectorStyle {
public:
    SymbolVectorStyle(std::shared_ptr<Value> textSize,
                      std::shared_ptr<Value> textColor,
                      std::shared_ptr<Value> textHaloColor,
                      std::shared_ptr<Value> textFont,
                      std::shared_ptr<Value> textField,
                      std::shared_ptr<Value> textTransform,
                      std::shared_ptr<Value> textOffset,
                      std::shared_ptr<Value> textRadialOffset,
                      std::shared_ptr<Value> textPadding,
                      std::shared_ptr<Value> textAnchor,
                      std::shared_ptr<Value> textVariableAnchor,
                      std::shared_ptr<Value> textRotate,
                      std::shared_ptr<Value> symbolSortKey,
                      std::shared_ptr<Value> symbolSpacing,
                      std::shared_ptr<Value> iconImage,
                      std::shared_ptr<Value> iconAnchor,
                      std::shared_ptr<Value> iconOffset,
                      std::shared_ptr<Value> iconSize,
                      std::shared_ptr<Value> textLineHeight,
                      std::shared_ptr<Value> textLetterSpacing,
                      double dpFactor):
    textSize(textSize),
    textFont(textFont),
    textField(textField),
    textTransform(textTransform),
    textOffset(textOffset),
    textRadialOffset(textRadialOffset),
    textColor(textColor),
    textHaloColor(textHaloColor),
    textPadding(textPadding),
    symbolSortKey(symbolSortKey),
    iconImage(iconImage),
    iconAnchor(iconAnchor),
    iconOffset(iconOffset),
    textAnchor(textAnchor),
    textVariableAnchor(textVariableAnchor),
    textRotate(textRotate),
    symbolSpacing(symbolSpacing),
    iconSize(iconSize),
    textLetterSpacing(textLetterSpacing),
    dpFactor(dpFactor) {}


    std::unordered_set<std::string> getUsedKeys() {

        std::unordered_set<std::string> usedKeys;
        std::vector<std::shared_ptr<Value>> values = {
            textSize, textFont, textField, textTransform, textOffset, textRadialOffset,
            textColor, textHaloColor, textPadding, symbolSortKey, iconImage,
            iconAnchor, iconOffset, textAnchor, textVariableAnchor, textRotate, symbolSpacing,
            iconSize, textLineHeight, textLetterSpacing
        };

        for (auto const &value: values) {
            if (!value) continue;
            auto const setKeys = value->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }

        return usedKeys;
    };

    double getTextSize(const EvaluationContext &context) {
        static const double defaultValue = 16.0;
        double value = textSize ? textSize->evaluateOr(context, defaultValue) : defaultValue;
        return value * dpFactor;
    }

    std::vector<std::string> getTextFont(const EvaluationContext &context) {
        static const std::vector<std::string> defaultValue = {"Open Sans Regular"};
        return textFont ? textFont->evaluateOr(context, defaultValue) : defaultValue;
    }

    std::vector<FormattedStringEntry> getTextField(const EvaluationContext &context) {
        static const std::vector<FormattedStringEntry> defaultValue;
        if (textField) {
            auto text = textField->evaluateOr(context, std::vector<FormattedStringEntry>());
            if (text.empty()) {
                auto string = ToStringValue(textField).evaluateOr(context, std::string(""));
                text.push_back({string, 1.0});
            }
            return text;
        }
        return defaultValue;
    }

    TextTransform getTextTransform(const EvaluationContext &context) {
        static const TextTransform defaultValue = TextTransform::NONE;
        return textTransform ? textTransform->evaluateOr(context, defaultValue) : defaultValue;
    }

    Vec2F getTextOffset(const EvaluationContext &context) {
        static const Vec2F defaultValue(0.0, 0.0);
        return textOffset ? textOffset->evaluateOr(context, defaultValue) : defaultValue;
    }

    double getTextRadialOffset(const EvaluationContext &context) {
        static const double defaultValue = 0.0;
        return textRadialOffset ? textRadialOffset->evaluateOr(context, defaultValue) : defaultValue;
    }

    Color getTextColor(const EvaluationContext &context){
        static const Color defaultValue = ColorUtil::c(0, 0, 0, 1.0);
        return textColor ? textColor->evaluateOr(context, defaultValue) : defaultValue;
    }

    Color getTextHaloColor(const EvaluationContext &context){
        static const Color defaultValue = ColorUtil::c(0, 0, 0, 0.0);
        return textHaloColor ? textHaloColor->evaluateOr(context, defaultValue) : defaultValue;
    }

    double getTextPadding(const EvaluationContext &context) {
        static const double defaultValue = 2.0;
        double value = textPadding ? textPadding->evaluateOr(context, defaultValue) : defaultValue;
        return value * dpFactor;
    }

    double getTextLetterSpacing(const EvaluationContext &context) {
        static const double defaultValue = 0.0;
        return textLetterSpacing ? textLetterSpacing->evaluateOr(context, defaultValue) : defaultValue;
    }

    int64_t getSymbolSortKey(const EvaluationContext &context) {
        static const int64_t defaultValue = 0.0;
        return symbolSortKey ? symbolSortKey->evaluateOr(context, defaultValue) : defaultValue;
    }

    std::string getIconImage(const EvaluationContext &context) {
        static const std::string defaultValue = "";
        return iconImage ? iconImage->evaluateOr(context, defaultValue) : defaultValue;
    }

    Anchor getIconAnchor(const EvaluationContext &context) {
        static const Anchor defaultValue = Anchor::CENTER;
        return iconAnchor ? iconAnchor->evaluateOr(context, defaultValue) : defaultValue;
    }

    Vec2F getIconOffset(const EvaluationContext &context) {
        static const Vec2F defaultValue(0.0, 0.0);
        return iconOffset ? iconOffset->evaluateOr(context, defaultValue) : defaultValue;
    }

    Anchor getTextAnchor(const EvaluationContext &context) {
        static const Anchor defaultValue = Anchor::CENTER;
        return textAnchor ? textAnchor->evaluateOr(context, defaultValue) : defaultValue;
    }

    TextJustify getTextJustify(const EvaluationContext &context) {
        static const TextJustify defaultValue = TextJustify::CENTER;
        return textAnchor ? textAnchor->evaluateOr(context, defaultValue) : defaultValue;
    }

    std::vector<Anchor> getTextVariableAnchor(const EvaluationContext &context) {
        static const std::vector<Anchor> defaultValue = {};
        return textVariableAnchor ? textVariableAnchor->evaluateOr(context, defaultValue) : defaultValue;
    }

    double getTextRotate(const EvaluationContext &context) {
        static const double defaultValue = 0.0;
        return textRotate ? textRotate->evaluateOr(context, defaultValue) : defaultValue;
    }

    double getSymbolSpacing(const EvaluationContext &context) {
        static const double defaultValue = 250.0;
        return symbolSpacing ? symbolSpacing->evaluateOr(context, defaultValue) : defaultValue;
    }

    double getIconSize(const EvaluationContext &context) {
        static const double defaultValue = 1;
        return iconSize ? iconSize->evaluateOr(context, defaultValue) : defaultValue;
    }

    double getTextLineHeight(const EvaluationContext &context) {
        static const double defaultValue = 1.2;
        return textLineHeight ? textLineHeight->evaluateOr(context, defaultValue) : defaultValue;
    }

private:
    std::shared_ptr<Value> textSize;
    std::shared_ptr<Value> textFont;
    std::shared_ptr<Value> textField;
    std::shared_ptr<Value> textTransform;
    std::shared_ptr<Value> textOffset;
    std::shared_ptr<Value> textRadialOffset;
    std::shared_ptr<Value> textColor;
    std::shared_ptr<Value> textHaloColor;
    std::shared_ptr<Value> textPadding;
    std::shared_ptr<Value> textAnchor;
    std::shared_ptr<Value> textVariableAnchor;
    std::shared_ptr<Value> textRotate;
    std::shared_ptr<Value> textLineHeight;
    std::shared_ptr<Value> textLetterSpacing;
    std::shared_ptr<Value> textJustify;
    std::shared_ptr<Value> symbolSortKey;
    std::shared_ptr<Value> symbolSpacing;
    std::shared_ptr<Value> iconImage;
    std::shared_ptr<Value> iconAnchor;
    std::shared_ptr<Value> iconOffset;
    std::shared_ptr<Value> iconSize;
    double dpFactor;


};

class SymbolVectorLayerDescription: public VectorLayerDescription {
public:
    VectorLayerType getType() override { return VectorLayerType::symbol; };
    SymbolVectorStyle style;

    SymbolVectorLayerDescription(std::string identifier,
                                 std::string source,
                                 std::string sourceId,
                               int minZoom,
                               int maxZoom,
                               std::shared_ptr<Value> filter,
                               SymbolVectorStyle style):
    VectorLayerDescription(identifier, source, sourceId, minZoom, maxZoom, filter),
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
