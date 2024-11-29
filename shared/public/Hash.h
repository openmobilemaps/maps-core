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

#include <cstdint>
#include <string>

// https://gist.github.com/Lee-R/3839813

namespace detail
{
    // FNV-1a 32bit hashing algorithm.
    inline constexpr uint32_t fnv1a_32(char const *s, size_t count) {
        return count ? (fnv1a_32(s, count - 1) ^ s[count - 1]) * 16777619u : 2166136261u;
    }
}    // namespace detail

constexpr uint32_t const_hash(char const* s, size_t count)
{
    return detail::fnv1a_32(s, count);
}

constexpr uint32_t const_hash(const std::string &s)
{
    return detail::fnv1a_32(s.c_str(), s.size());
}