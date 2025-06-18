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

static const std::uint64_t FNV1A_64_INIT = 0xcbf29ce484222325ULL;
static const std::uint64_t FNV1A_64_PRIME = 0x100000001b3ULL;

namespace fnv
{
    // FNV-1a 64bit hashing algorithm.
    inline constexpr uint64_t fnv1a_64(char const *s, size_t count) {
        return count ? (fnv1a_64(s, count - 1) ^ s[count - 1]) * FNV1A_64_PRIME : FNV1A_64_INIT;
    }
}    // namespace fnv

constexpr uint64_t const_hash(char const* s, size_t count)
{
    return fnv::fnv1a_64(s, count);
}

// Expects a zero terminated char array
constexpr uint64_t const_hash(char const* s)
{
    size_t size = 0;
    while (s[size] != '\0') {
        ++size;
    }
    return fnv::fnv1a_64(s, size);
}

constexpr uint64_t const_hash(const std::string &s)
{
    return fnv::fnv1a_64(s.c_str(), s.size());
}