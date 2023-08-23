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

#include <string>
#include <variant>
#include <vector>
#include "FormattedStringEntry.h"
#include "Color.h"

typedef std::variant<std::string,
                     double,
                     int64_t,
                     bool,
                     Color,
                     std::vector<float>,
                     std::vector<std::string>,
                     std::vector<FormattedStringEntry>,
                     std::monostate> ValueVariant;

// helper type for the visitor #4
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };

// explicit deduction guide (not needed as of C++20)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

