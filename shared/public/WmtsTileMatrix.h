// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from wmts_capabilities.djinni

#pragma once

#include <cstdint>
#include <string>
#include <utility>

struct WmtsTileMatrix final {
    std::string identifier;
    double scaleDenominator;
    double topLeftCornerX;
    double topLeftCornerY;
    int32_t tileWidth;
    int32_t tileHeight;
    int32_t matrixWidth;
    int32_t matrixHeight;

    WmtsTileMatrix(std::string identifier_,
                   double scaleDenominator_,
                   double topLeftCornerX_,
                   double topLeftCornerY_,
                   int32_t tileWidth_,
                   int32_t tileHeight_,
                   int32_t matrixWidth_,
                   int32_t matrixHeight_)
    : identifier(std::move(identifier_))
    , scaleDenominator(std::move(scaleDenominator_))
    , topLeftCornerX(std::move(topLeftCornerX_))
    , topLeftCornerY(std::move(topLeftCornerY_))
    , tileWidth(std::move(tileWidth_))
    , tileHeight(std::move(tileHeight_))
    , matrixWidth(std::move(matrixWidth_))
    , matrixHeight(std::move(matrixHeight_))
    {}
};
