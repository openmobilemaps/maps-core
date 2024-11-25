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

#include "Color.h"
#include "FormattedStringEntry.h"
#include <iomanip>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

// never change order!!!!!
typedef std::variant<std::string, double, int64_t, bool, Color, std::vector<float>, std::vector<std::string>,
                     std::vector<FormattedStringEntry>, std::monostate>
    ValueVariant;

// helper type for the visitor #4
template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};

// explicit deduction guide (not needed as of C++20)
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// implement custom == operator for ValueVariant
inline bool operator==(const ValueVariant &lhs, const ValueVariant &rhs) {
    return std::visit(
        overloaded{

            // Same types
            [](const std::string &lhs, const std::string &rhs) { return lhs == rhs; },
            [](double lhs, double rhs) { return lhs == rhs; }, [](int64_t lhs, int64_t rhs) { return lhs == rhs; },
            [](bool lhs, bool rhs) { return lhs == rhs; }, [](const Color &lhs, const Color &rhs) { return lhs == rhs; },
            [](const std::vector<float> &lhs, const std::vector<float> &rhs) { return lhs == rhs; },
            [](const std::vector<std::string> &lhs, const std::vector<std::string> &rhs) { return lhs == rhs; },
            [](const std::vector<FormattedStringEntry> &lhs, const std::vector<FormattedStringEntry> &rhs) { return lhs == rhs; },

            // Double-int and int-double
            [](double lhs, int64_t rhs) { return lhs == rhs; }, [](int64_t lhs, double rhs) { return lhs == rhs; },

            // Bool and numbers
            [](bool lhs, double rhs) { return lhs == rhs; }, [](double lhs, bool rhs) { return lhs == rhs; },
            [](bool lhs, int64_t rhs) { return lhs == rhs; }, [](int64_t lhs, bool rhs) { return lhs == rhs; },

            // Monostate
            [](const std::monostate &lhs, const std::monostate &rhs) { return true; },

            // No match
            [](const auto &lhs, const auto &rhs) { return false; }},
        lhs, rhs);
};

const static auto valueVariantToStringVisitor =
    overloaded{[](const std::string &val) { return val; },
               [](double val) {
                   // strip all decimal places
                   return std::to_string((int64_t)val);
               },
               [](int64_t val) { return std::to_string(val); },
               [](bool val) { return val ? std::string("true") : std::string("false"); },
               [](const Color &val) {
                   std::stringstream stream;
                   stream << "#" << std::hex << ((uint8_t)(val.r * 255)) << ((uint8_t)(val.g * 255)) << ((uint8_t)(val.b * 255))
                          << ((uint8_t)(val.a * 255));
                   return stream.str();
               },
               [](const std::vector<float> &val) {
                   std::stringstream stream;
                   stream << std::setprecision(4) << "[";
                   for (auto iter = val.begin(); iter != val.end(); iter++) {
                       stream << *iter << (iter + 1 == val.end() ? "]" : ", ");
                   }
                   return stream.str();
               },
               [](const std::vector<std::string> &val) {
                   std::stringstream stream;
                   stream << "[";
                   for (auto iter = val.begin(); iter != val.end(); iter++) {
                       stream << *iter << (iter + 1 == val.end() ? "]" : ", ");
                   }
                   return stream.str();
               },
               [](const std::vector<FormattedStringEntry> &val) {
                   std::stringstream stream;
                   for (auto const &v : val) {
                       stream << v.text;
                   }
                   return stream.str();
               },
               [](const std::monostate &val) { return std::string(""); }};
