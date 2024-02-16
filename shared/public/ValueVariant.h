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
#include <iomanip>
#include <sstream>
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

const static auto valueVariantToStringVisitor = overloaded {
        [](const std::string &val){
            return val;
        },
        [](double val){
            // strip all decimal places
            return std::to_string((int64_t)val);
        },
        [](int64_t val){
            return std::to_string(val);
        },
        [](bool val){
            return val ? std::string("true") : std::string("false");
        },
        [](const Color &val){
            std::stringstream stream;
            stream << "#" << std::hex << ((uint8_t) (val.r * 255)) << ((uint8_t) (val.g * 255)) << ((uint8_t) (val.b * 255)) << ((uint8_t) (val.a * 255));
            return stream.str();
        },
        [](const std::vector<float> &val){
            std::stringstream stream;
            stream << std::setprecision(4) << "[";
            for (auto iter = val.begin(); iter != val.end(); iter++) {
                stream << *iter << (iter + 1 == val.end() ? "]" : ", ");
            }
            return stream.str();
        },
        [](const std::vector<std::string> &val){
            std::stringstream stream;
            stream << "[";
            for (auto iter = val.begin(); iter != val.end(); iter++) {
                stream << *iter << (iter + 1 == val.end() ? "]" : ", ");
            }
            return stream.str();
        },
        [](const std::vector<FormattedStringEntry> &val){
            std::stringstream stream;
            for (auto const &v: val) {
                stream << v.text;
            }
            return stream.str();
        },
        [](const std::monostate &val) {
            return std::string("");
        }
};
