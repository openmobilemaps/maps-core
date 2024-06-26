// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from line.djinni

#pragma once

#include <utility>

struct ShaderLineStyle final {
    float width;
    float colorR;
    float colorG;
    float colorB;
    float colorA;
    float gapColorR;
    float gapColorG;
    float gapColorB;
    float gapColorA;
    float widthAsPixel;
    float opacity;
    float blur;
    float lineCap;
    float numDashValue;
    float dashValue0;
    float dashValue1;
    float dashValue2;
    float dashValue3;
    float offset;
    float dotted;
    float dottedSkew;

    ShaderLineStyle(float width_,
                    float colorR_,
                    float colorG_,
                    float colorB_,
                    float colorA_,
                    float gapColorR_,
                    float gapColorG_,
                    float gapColorB_,
                    float gapColorA_,
                    float widthAsPixel_,
                    float opacity_,
                    float blur_,
                    float lineCap_,
                    float numDashValue_,
                    float dashValue0_,
                    float dashValue1_,
                    float dashValue2_,
                    float dashValue3_,
                    float offset_,
                    float dotted_,
                    float dottedSkew_)
    : width(std::move(width_))
    , colorR(std::move(colorR_))
    , colorG(std::move(colorG_))
    , colorB(std::move(colorB_))
    , colorA(std::move(colorA_))
    , gapColorR(std::move(gapColorR_))
    , gapColorG(std::move(gapColorG_))
    , gapColorB(std::move(gapColorB_))
    , gapColorA(std::move(gapColorA_))
    , widthAsPixel(std::move(widthAsPixel_))
    , opacity(std::move(opacity_))
    , blur(std::move(blur_))
    , lineCap(std::move(lineCap_))
    , numDashValue(std::move(numDashValue_))
    , dashValue0(std::move(dashValue0_))
    , dashValue1(std::move(dashValue1_))
    , dashValue2(std::move(dashValue2_))
    , dashValue3(std::move(dashValue3_))
    , offset(std::move(offset_))
    , dotted(std::move(dotted_))
    , dottedSkew(std::move(dottedSkew_))
    {}
};
