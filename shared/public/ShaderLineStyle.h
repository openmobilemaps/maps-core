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
#ifndef __APPLE__
    HalfFloat padding; // Padding to have the size as a multiple of 16 bytes (OpenGL uniform buffer padding due to std140)
#endif
};

#ifdef __APPLE__
static_assert(sizeof(ShaderLineStyle) == 46, "ShaderLineStyle struct is not packed correctly!");
#else
static_assert(sizeof(ShaderLineStyle) == 96 && sizeof(ShaderLineStyle) % 16 == 0, "ShaderLineStyle struct is not packed correctly!");
#endif

