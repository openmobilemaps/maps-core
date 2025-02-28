#pragma once

#include <utility>
#include "HalfFloat.h"

struct __attribute__((packed)) ShaderLineStyle final {
    HalfFloat width;
    HalfFloat colorR;
    HalfFloat colorG;
    HalfFloat colorB;
    HalfFloat colorA;
    HalfFloat gapColorR;
    HalfFloat gapColorG;
    HalfFloat gapColorB;
    HalfFloat gapColorA;
    HalfFloat widthAsPixel;
    HalfFloat opacity;
    HalfFloat blur;
    HalfFloat lineCap;
    HalfFloat numDashValue;
    HalfFloat dashValue0;
    HalfFloat dashValue1;
    HalfFloat dashValue2;
    HalfFloat dashValue3;
    HalfFloat dashFade;
    HalfFloat dashAnimationSpeed;
    HalfFloat offset;
    HalfFloat dotted;
    HalfFloat dottedSkew;
};

#ifdef __APPLE__
static_assert(sizeof(ShaderLineStyle) == 46, "ShaderLineStyle struct is not packed correctly!");
#else
static_assert(sizeof(ShaderLineStyle) == 92, "ShaderLineStyle struct is not packed correctly!");
#endif

