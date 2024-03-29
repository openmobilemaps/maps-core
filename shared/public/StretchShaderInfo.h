// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from shader.djinni

#pragma once

#include "RectD.h"
#include <utility>

struct StretchShaderInfo final {
    float scaleX;
    /** all stretch infos are between 0 and 1 */
    float stretchX0Begin;
    float stretchX0End;
    float stretchX1Begin;
    float stretchX1End;
    float scaleY;
    /** all stretch infos are between 0 and 1 */
    float stretchY0Begin;
    float stretchY0End;
    float stretchY1Begin;
    float stretchY1End;
    ::RectD uv;

    StretchShaderInfo(float scaleX_,
                      float stretchX0Begin_,
                      float stretchX0End_,
                      float stretchX1Begin_,
                      float stretchX1End_,
                      float scaleY_,
                      float stretchY0Begin_,
                      float stretchY0End_,
                      float stretchY1Begin_,
                      float stretchY1End_,
                      ::RectD uv_)
    : scaleX(std::move(scaleX_))
    , stretchX0Begin(std::move(stretchX0Begin_))
    , stretchX0End(std::move(stretchX0End_))
    , stretchX1Begin(std::move(stretchX1Begin_))
    , stretchX1End(std::move(stretchX1End_))
    , scaleY(std::move(scaleY_))
    , stretchY0Begin(std::move(stretchY0Begin_))
    , stretchY0End(std::move(stretchY0End_))
    , stretchY1Begin(std::move(stretchY1Begin_))
    , stretchY1End(std::move(stretchY1End_))
    , uv(std::move(uv_))
    {}
};
