//
//  Vec4D.h
//  MapCore
//
//  Created by Marco Zimmermann on 21.10.2024.
//

#pragma once

#include <utility>

struct Vec4D final {
    double x;
    double y;
    double z;
    double w;

    Vec4D(double x_,
          double y_,
          double z_,
          double w_)
    : x(std::move(x_))
    , y(std::move(y_))
    , z(std::move(z_))
    , w(std::move(w_))
    {}
};
