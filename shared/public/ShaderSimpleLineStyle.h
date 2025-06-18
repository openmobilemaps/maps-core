#pragma once

#include <utility>
#include "HalfFloat.h"

struct __attribute__((packed)) ShaderSimpleLineStyle final {
    HalfFloat width;
    HalfFloat colorR;
    HalfFloat colorG;
    HalfFloat colorB;
    HalfFloat colorA;
    HalfFloat widthAsPixel;
    HalfFloat opacity;
    HalfFloat lineCap;
#ifndef __APPLE__
    // Ensure structure size is multiple of 16 bytes due to OpenGL uniform buffer padding rules (std140)
#endif
};

#ifdef __APPLE__
static_assert(sizeof(ShaderSimpleLineStyle) == 16, "ShaderSimpleLineStyle struct is not packed correctly!");
#else
static_assert(sizeof(ShaderSimpleLineStyle) == 32, "ShaderSimpleLineStyle struct is not packed correctly!");
#endif
