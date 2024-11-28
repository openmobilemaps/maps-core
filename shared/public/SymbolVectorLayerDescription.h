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
#include "TextSymbolPlacement.h"
#include "SymbolAlignment.h"
#include "ColorUtil.h"
#include "IconTextFit.h"
#include "SymbolZOrder.h"

class SymbolVectorStyle {
public:
    SymbolVectorStyle(std::shared_ptr<Value> textSize,
                      std::shared_ptr<Value> textColor,
                      std::shared_ptr<Value> textHaloColor,
                      std::shared_ptr<Value> textHaloWidth,
                      std::shared_ptr<Value> textHaloBlur,
                      std::shared_ptr<Value> textOpacity,
                      std::shared_ptr<Value> textFont,
                      std::shared_ptr<Value> textField,
                      std::shared_ptr<Value> textTransform,
                      std::shared_ptr<Value> textOffset,
                      std::shared_ptr<Value> textRadialOffset,
                      std::shared_ptr<Value> textPadding,
                      std::shared_ptr<Value> textAnchor,
                      std::shared_ptr<Value> textJustify,
                      std::shared_ptr<Value> textVariableAnchor,
                      std::shared_ptr<Value> textRotate,
                      std::shared_ptr<Value> textAllowOverlap,
                      std::shared_ptr<Value> textRotationAlignment,
                      std::shared_ptr<Value> textOptional,
                      std::shared_ptr<Value> symbolSortKey,
                      std::shared_ptr<Value> symbolSpacing,
                      std::shared_ptr<Value> symbolPlacement,
                      std::shared_ptr<Value> iconRotationAlignment,
                      std::shared_ptr<Value> iconImage,
                      std::shared_ptr<Value> iconImageCustomProvider,
                      std::shared_ptr<Value> iconAnchor,
                      std::shared_ptr<Value> iconOffset,
                      std::shared_ptr<Value> iconSize,
                      std::shared_ptr<Value> iconAllowOverlap,
                      std::shared_ptr<Value> iconPadding,
                      std::shared_ptr<Value> iconTextFit,
                      std::shared_ptr<Value> iconTextFitPadding,
                      std::shared_ptr<Value> iconOpacity,
                      std::shared_ptr<Value> iconRotate,
                      std::shared_ptr<Value> iconOptional,
                      std::shared_ptr<Value> textLineHeight,
                      std::shared_ptr<Value> textLetterSpacing,
                      std::shared_ptr<Value> textMaxWidth,
                      std::shared_ptr<Value> textMaxAngle,
                      std::shared_ptr<Value> blendMode,
                      std::shared_ptr<Value> symbolZOrder,
                      int64_t transitionDuration,
                      int64_t transitionDelay):
    textSizeEvaluator(textSize),
    textFontEvaluator(textFont),
    textFieldEvaluator(textField),
    textTransformEvaluator(textTransform),
    textOffsetEvaluator(textOffset),
    textRadialOffsetEvaluator(textRadialOffset),
    textColorEvaluator(textColor),
    textHaloColorEvaluator(textHaloColor),
    textHaloWidthEvaluator(textHaloWidth),
    textHaloBlurEvaluator(textHaloBlur),
    textPaddingEvaluator(textPadding),
    symbolSortKeyEvaluator(symbolSortKey),
    symbolPlacementEvaluator(symbolPlacement),
    iconImageEvaluator(iconImage),
    iconImageCustomProviderEvaluator(iconImageCustomProvider),
    iconAnchorEvaluator(iconAnchor),
    iconOffsetEvaluator(iconOffset),
    textAnchorEvaluator(textAnchor),
    textJustifyEvaluator(textJustify),
    textVariableAnchorEvaluator(textVariableAnchor),
    textOptionalEvaluator(textOptional),
    textRotateEvaluator(textRotate),
    textOpacityEvaluator(textOpacity),
    symbolSpacingEvaluator(symbolSpacing),
    iconSizeEvaluator(iconSize),
    textLetterSpacingEvaluator(textLetterSpacing),
    textAllowOverlapEvaluator(textAllowOverlap),
    textMaxWidthEvaluator(textMaxWidth),
    textMaxAngleEvaluator(textMaxAngle),
    iconAllowOverlapEvaluator(iconAllowOverlap),
    iconPaddingEvaluator(iconPadding),
    iconTextFitEvaluator(iconTextFit),
    iconTextFitPaddingEvaluator(iconTextFitPadding),
    iconOpacityEvaluator(iconOpacity),
    iconRotateEvaluator(iconRotate),
    iconOptionalEvaluator(iconOptional),
    textLineHeightEvaluator(textLineHeight),
    textRotationAlignmentEvaluator(textRotationAlignment),
    iconRotationAlignmentEvaluator(iconRotationAlignment),
    blendModeEvaluator(blendMode),
    symbolZOrderEvaluator(symbolZOrder),
    transitionDuration(transitionDuration),
    transitionDelay(transitionDelay) {}

    SymbolVectorStyle(SymbolVectorStyle &style)
            : textSizeEvaluator(style.textSizeEvaluator),
              textFontEvaluator(style.textFontEvaluator),
              textFieldEvaluator(style.textFieldEvaluator),
              textTransformEvaluator(style.textTransformEvaluator),
              textOffsetEvaluator(style.textOffsetEvaluator),
              textRadialOffsetEvaluator(style.textRadialOffsetEvaluator),
              textColorEvaluator(style.textColorEvaluator),
              textHaloColorEvaluator(style.textHaloColorEvaluator),
              textHaloWidthEvaluator(style.textHaloWidthEvaluator),
              textHaloBlurEvaluator(style.textHaloBlurEvaluator),
              textPaddingEvaluator(style.textPaddingEvaluator),
              symbolSortKeyEvaluator(style.symbolSortKeyEvaluator),
              symbolPlacementEvaluator(style.symbolPlacementEvaluator),
              iconImageEvaluator(style.iconImageEvaluator),
              iconImageCustomProviderEvaluator(style.iconImageCustomProviderEvaluator),
              iconAnchorEvaluator(style.iconAnchorEvaluator),
              iconOffsetEvaluator(style.iconOffsetEvaluator),
              textAnchorEvaluator(style.textAnchorEvaluator),
              textJustifyEvaluator(style.textJustifyEvaluator),
              textVariableAnchorEvaluator(style.textVariableAnchorEvaluator),
              textOptionalEvaluator(style.textOptionalEvaluator),
              textRotateEvaluator(style.textRotateEvaluator),
              textOpacityEvaluator(style.textOpacityEvaluator),
              symbolSpacingEvaluator(style.symbolSpacingEvaluator),
              iconSizeEvaluator(style.iconSizeEvaluator),
              textLineHeightEvaluator(style.textLineHeightEvaluator),
              textLetterSpacingEvaluator(style.textLetterSpacingEvaluator),
              textAllowOverlapEvaluator(style.textAllowOverlapEvaluator),
              textMaxWidthEvaluator(style.textMaxWidthEvaluator),
              textMaxAngleEvaluator(style.textMaxAngleEvaluator),
              iconAllowOverlapEvaluator(style.iconAllowOverlapEvaluator),
              iconPaddingEvaluator(style.iconPaddingEvaluator),
              iconTextFitEvaluator(style.iconTextFitEvaluator),
              iconTextFitPaddingEvaluator(style.iconTextFitPaddingEvaluator),
              iconOpacityEvaluator(style.iconOpacityEvaluator),
              iconRotateEvaluator(style.iconRotateEvaluator),
              iconOptionalEvaluator(style.iconOptionalEvaluator),
              textRotationAlignmentEvaluator(style.textRotationAlignmentEvaluator),
              iconRotationAlignmentEvaluator(style.iconRotationAlignmentEvaluator),
              blendModeEvaluator(style.blendModeEvaluator),
              symbolZOrderEvaluator(style.symbolZOrderEvaluator),
              transitionDuration(style.transitionDuration),
              transitionDelay(style.transitionDelay) {

                  if(symbolSortKeyEvaluator.getValue() != nullptr) {
                      auto usedKeysCollection = symbolSortKeyEvaluator.getValue()->getUsedKeys();
                      auto isZoomDependent = usedKeysCollection.usedKeys.contains("zoom");
                      auto isStateDependant = usedKeysCollection.isStateDependant();
                      symbolSortKeyIsIndependent = !isZoomDependent && !isStateDependant;
                  }
              }

    UsedKeysCollection getUsedKeys() const {

        UsedKeysCollection usedKeys;
        std::shared_ptr<Value> values[] = {
            textSizeEvaluator.getValue(), textFontEvaluator.getValue(), textFieldEvaluator.getValue(), textTransformEvaluator.getValue(), textOffsetEvaluator.getValue(), textRadialOffsetEvaluator.getValue(),
            textColorEvaluator.getValue(), textHaloColorEvaluator.getValue(), textHaloWidthEvaluator.getValue(), textHaloBlurEvaluator.getValue(), textPaddingEvaluator.getValue(), symbolSortKeyEvaluator.getValue(), symbolPlacementEvaluator.getValue(), iconImageEvaluator.getValue(),
            iconAnchorEvaluator.getValue(), iconOffsetEvaluator.getValue(), textAnchorEvaluator.getValue(), textVariableAnchorEvaluator.getValue(), textRotateEvaluator.getValue(), symbolSpacingEvaluator.getValue(),
            iconSizeEvaluator.getValue(), textLineHeightEvaluator.getValue(), textLetterSpacingEvaluator.getValue(), textAllowOverlapEvaluator.getValue(), iconAllowOverlapEvaluator.getValue(),
            iconPaddingEvaluator.getValue(), textOpacityEvaluator.getValue(), iconOpacityEvaluator.getValue(), iconRotationAlignmentEvaluator.getValue(), textRotationAlignmentEvaluator.getValue(),
            iconTextFitEvaluator.getValue(), iconTextFitPaddingEvaluator.getValue(), textMaxWidthEvaluator.getValue(), textMaxAngleEvaluator.getValue(), iconRotateEvaluator.getValue(), blendModeEvaluator.getValue(),
            textOptionalEvaluator.getValue(), iconOptionalEvaluator.getValue(), symbolZOrderEvaluator.getValue(), iconImageCustomProviderEvaluator.getValue()
        };

        for (auto const &value: values) {
            if (!value) continue;
            auto const setKeys = value->getUsedKeys();
            usedKeys.includeOther(setKeys);
        }

        return usedKeys;
    };

    BlendMode getBlendMode(const EvaluationContext &context) {
        static const BlendMode defaultValue = BlendMode::NORMAL;
        return blendModeEvaluator.getResult(context, defaultValue);
    }

    double getTextSize(const EvaluationContext &context) {
        static const double defaultValue = 16.0;
        double value = textSizeEvaluator.getResult(context, defaultValue);
        return value * context.dpFactor;
    }

    std::vector<std::string> getTextFont(const EvaluationContext &context) {
        static const std::vector<std::string> defaultValue = {"Open Sans Regular"};
        return textFontEvaluator.getResult(context, defaultValue);
    }

    std::vector<FormattedStringEntry> getTextField(const EvaluationContext &context) {
        static const std::vector<FormattedStringEntry> defaultValue;

        auto textField = textFieldEvaluator.getValue();
        if (textField) {
            auto text = textFieldEvaluator.getResult(context, defaultValue);
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
        return textTransformEvaluator.getResult(context, defaultValue);
    }

    Vec2F getTextOffset(const EvaluationContext &context) {
        static const Vec2F defaultValue(0.0, 0.0);
        return textOffsetEvaluator.getResult(context, defaultValue);
    }

    double getTextRadialOffset(const EvaluationContext &context) {
        static const double defaultValue = 0.0;
        return textRadialOffsetEvaluator.getResult(context, defaultValue);
    }

    Color getTextColor(const EvaluationContext &context){
        static const Color defaultValue = ColorUtil::c(0, 0, 0, 1.0);
        return textColorEvaluator.getResult(context, defaultValue);
    }

    Color getTextHaloColor(const EvaluationContext &context){
        static const Color defaultValue = ColorUtil::c(0.0, 0.0, 0.0, 0.0);
        return textHaloColorEvaluator.getResult(context, defaultValue);
    }

    double getTextHaloWidth(const EvaluationContext &context, double size) {
        static double defaultValue = 0.0;
        double width = textHaloWidthEvaluator.getResult(context, defaultValue);

        // in a font of size 41pt, we can show around 7pt of halo
        // (due to generation of font atlasses)
        double relativeMax = 7.0 / 41.0;
        double relative = width / size;

        return std::max(0.0, std::min(1.0, relative / relativeMax));
    }

    double getTextHaloBlur(const EvaluationContext &context) {
        static double defaultValue = 0.0;
        double width = textHaloBlurEvaluator.getResult(context, defaultValue);

        double size = getTextSize(context);

        // in a font of size 41pt, we can show around 7pt of halo
        // (due to generation of font atlasses)
        double relativeMax = 7.0 / 41.0;
        double relative = width / size;

        return std::max(0.0, std::min(1.0, relative / relativeMax));
    }

    double getTextPadding(const EvaluationContext &context) {
        static const double defaultValue = 2.0;
        double value = textPaddingEvaluator.getResult(context, defaultValue);
        return value * context.dpFactor;
    }

    double getIconPadding(const EvaluationContext &context) {
        static const double defaultValue = 2.0;
        double value = iconPaddingEvaluator.getResult(context, defaultValue);
        return value * context.dpFactor;
    }

    double getTextLetterSpacing(const EvaluationContext &context) {
        static const double defaultValue = 0.0;
        return textLetterSpacingEvaluator.getResult(context, defaultValue);
    }

    double getTextOpacity(const EvaluationContext &context) {
        static const double defaultValue = 1.0;
        return textOpacityEvaluator.getResult(context, defaultValue);
    }

    double getIconOpacity(const EvaluationContext &context) {
        static const double defaultValue = 1.0;
        return iconOpacityEvaluator.getResult(context, defaultValue);
    }

    bool getTextAllowOverlap(const EvaluationContext &context) {
        static const bool defaultValue = false;
        return textAllowOverlapEvaluator.getResult(context, defaultValue);
    }

    double getSymbolSortKey(const EvaluationContext &context) {
        static const double defaultValue = 0.0;
        return symbolSortKeyEvaluator.getResult(context, defaultValue);
    }

    std::string getIconImage(const EvaluationContext &context) {
        static const std::string defaultValue = "";
        return iconImageEvaluator.getResult(context, defaultValue);
    }

    bool getIconImageCustomProvider(const EvaluationContext &context) {
        static const bool defaultValue = false;
        return iconImageCustomProviderEvaluator.getResult(context, defaultValue);
    }

    bool hasIconImagePotentially() {
        return (iconImageEvaluator.getValue() || iconImageCustomProviderEvaluator.getValue()) ? true : false;
    }

    Anchor getIconAnchor(const EvaluationContext &context) {
        static const Anchor defaultValue = Anchor::CENTER;
        return iconAnchorEvaluator.getResult(context, defaultValue);
    }

    Vec2F getIconOffset(const EvaluationContext &context) {
        static const Vec2F defaultValue(0.0, 0.0);
        const auto result = iconOffsetEvaluator.getResult(context, defaultValue);
        return Vec2F(result.x * context.dpFactor, result.y * context.dpFactor);
    }

    bool getIconOptional(const EvaluationContext &context) {
        static const bool defaultValue = false;
        return iconOptionalEvaluator.getResult(context, defaultValue);
    }

    bool getTextOptional(const EvaluationContext &context) {
        static const bool defaultValue = false;
        return textOptionalEvaluator.getResult(context, defaultValue);
    }

    Anchor getTextAnchor(const EvaluationContext &context) {
        static const Anchor defaultValue = Anchor::CENTER;
        return textAnchorEvaluator.getResult(context, defaultValue);
    }

    TextJustify getTextJustify(const EvaluationContext &context) {
        static const TextJustify defaultValue = TextJustify::CENTER;
        return textJustifyEvaluator.getResult(context, defaultValue);
    }

    TextSymbolPlacement getTextSymbolPlacement(const EvaluationContext &context) {
        static const auto defaultValue = TextSymbolPlacement::POINT;
        return symbolPlacementEvaluator.getResult(context, defaultValue);
    }

    std::vector<Anchor> getTextVariableAnchor(const EvaluationContext &context) {
        static const std::vector<Anchor> defaultValue = {};
        return textVariableAnchorEvaluator.getResult(context, defaultValue);
    }

    double getTextRotate(const EvaluationContext &context) {
        static const double defaultValue = 0.0;
        return textRotateEvaluator.getResult(context, defaultValue);
    }

    double getIconRotate(const EvaluationContext &context) {
        static const double defaultValue = 0.0;
        return iconRotateEvaluator.getResult(context, defaultValue);
    }

    double getSymbolSpacing(const EvaluationContext &context) {
        static const double defaultValue = 250.0;
        return symbolSpacingEvaluator.getResult(context, defaultValue);
    }

    double getIconSize(const EvaluationContext &context) {
        static const double defaultValue = 1;
        return iconSizeEvaluator.getResult(context, defaultValue);
    }

    bool getIconAllowOverlap(const EvaluationContext &context) {
        static const bool defaultValue = false;
        return iconAllowOverlapEvaluator.getResult(context, defaultValue);
    }

    double getTextLineHeight(const EvaluationContext &context) {
        static const double defaultValue = 1.2;
        return textLineHeightEvaluator.getResult(context, defaultValue);
    }

    int64_t getTextMaxWidth(const EvaluationContext &context) {
        static const int64_t averageCharacterWidth = 2.0;
        static const int64_t defaultValue = 10;
        return textMaxWidthEvaluator.getResult(context, defaultValue) * averageCharacterWidth;
    }

    double getTextMaxAngle(const EvaluationContext &context) {
        static const double defaultValue = 45.0f;
        return textMaxAngleEvaluator.getResult(context, defaultValue);
    }

    SymbolAlignment getTextRotationAlignment(const EvaluationContext &context) {
        static const SymbolAlignment defaultValue = SymbolAlignment::AUTO;
        return textRotationAlignmentEvaluator.getResult(context, defaultValue);
    }

    SymbolAlignment getIconRotationAlignment(const EvaluationContext &context) {
        static const SymbolAlignment defaultValue = SymbolAlignment::AUTO;
        return iconRotationAlignmentEvaluator.getResult(context, defaultValue);
    }

    IconTextFit getIconTextFit(const EvaluationContext &context) {
        static const IconTextFit defaultValue = IconTextFit::NONE;
        return iconTextFitEvaluator.getResult(context, defaultValue);
    }

    //top, right, bottom, left
    std::vector<float> getIconTextFitPadding(const EvaluationContext &context) {
        static const std::vector<float> defaultValue{0.0, 0.0, 0.0, 0.0};
        return iconTextFitPaddingEvaluator.getResult(context, defaultValue);
    }

    SymbolZOrder getSymbolZOrder(const EvaluationContext &context) {
        static const SymbolZOrder defaultValue = SymbolZOrder::AUTO;
        return symbolZOrderEvaluator.getResult(context, defaultValue);
    }

    const int64_t getTransitionDuration() const {
        return transitionDuration;
    }

    const int64_t getTransitionDelay() const {
        return transitionDelay;
    }

    const bool symbolSortKeyNeedsRecomputation() const {
        return !symbolSortKeyIsIndependent;
    }

    int64_t transitionDuration;
    int64_t transitionDelay;

private:
    ValueEvaluator<BlendMode> blendModeEvaluator;
    ValueEvaluator<double> textSizeEvaluator;
    ValueEvaluator<std::vector<std::string>> textFontEvaluator;
    ValueEvaluator<std::vector<FormattedStringEntry>> textFieldEvaluator;
    ValueEvaluator<TextTransform> textTransformEvaluator;
    ValueEvaluator<Vec2F> textOffsetEvaluator;
    ValueEvaluator<double> textRadialOffsetEvaluator;
    ValueEvaluator<Color> textColorEvaluator;
    ValueEvaluator<Color> textHaloColorEvaluator;
    ValueEvaluator<double> textHaloWidthEvaluator;
    ValueEvaluator<double> textHaloBlurEvaluator;
    ValueEvaluator<double> textPaddingEvaluator;
    ValueEvaluator<double> iconPaddingEvaluator;
    ValueEvaluator<double> textLetterSpacingEvaluator;
    ValueEvaluator<double> textOpacityEvaluator;
    ValueEvaluator<double> iconOpacityEvaluator;
    ValueEvaluator<bool> textAllowOverlapEvaluator;
    ValueEvaluator<double> symbolSortKeyEvaluator;
    ValueEvaluator<std::string> iconImageEvaluator;
    ValueEvaluator<bool> iconImageCustomProviderEvaluator;
    ValueEvaluator<Anchor> iconAnchorEvaluator;
    ValueEvaluator<Vec2F> iconOffsetEvaluator;
    ValueEvaluator<bool> iconOptionalEvaluator;
    ValueEvaluator<bool> textOptionalEvaluator;
    ValueEvaluator<Anchor> textAnchorEvaluator;
    ValueEvaluator<TextJustify> textJustifyEvaluator;
    ValueEvaluator<TextSymbolPlacement> symbolPlacementEvaluator;
    ValueEvaluator<std::vector<Anchor>> textVariableAnchorEvaluator;
    ValueEvaluator<double> textRotateEvaluator;
    ValueEvaluator<double> iconRotateEvaluator;
    ValueEvaluator<double> symbolSpacingEvaluator;
    ValueEvaluator<double> iconSizeEvaluator;
    ValueEvaluator<bool> iconAllowOverlapEvaluator;
    ValueEvaluator<double> textLineHeightEvaluator;
    ValueEvaluator<int64_t> textMaxWidthEvaluator;
    ValueEvaluator<double> textMaxAngleEvaluator;
    ValueEvaluator<SymbolAlignment> textRotationAlignmentEvaluator;
    ValueEvaluator<SymbolAlignment> iconRotationAlignmentEvaluator;
    ValueEvaluator<IconTextFit> iconTextFitEvaluator;
    ValueEvaluator<std::vector<float>> iconTextFitPaddingEvaluator;
    ValueEvaluator<SymbolZOrder> symbolZOrderEvaluator;

    bool symbolSortKeyIsIndependent = true;
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
                                 SymbolVectorStyle style,
                                 std::optional<int32_t> renderPassIndex,
                                 std::shared_ptr<Value> interactable,
                                 bool selfMasked):
    VectorLayerDescription(identifier, source, sourceId, minZoom, maxZoom, filter, renderPassIndex, interactable, false, selfMasked),
    style(style) {};

    std::unique_ptr<VectorLayerDescription> clone() override {
        return std::make_unique<SymbolVectorLayerDescription>(identifier, source, sourceLayer, minZoom, maxZoom,
                                                              filter ? filter->clone() : nullptr, style, renderPassIndex,
                                                              interactable ? interactable : nullptr, selfMasked);
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
