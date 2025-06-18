/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "RectanglePacker.h"
#include "RectI.h"
#include "dp_rect_pack.h"
#include "HashedTuple.h"
#include "RectanglePackerPage.h"
#include <algorithm>


std::vector<RectanglePackerPage> RectanglePacker::pack(const std::unordered_map<std::string, ::Vec2I> & rectangles, const ::Vec2I & maxPageSize) {

    std::unordered_map<size_t, RectanglePackerPage> results;
    dp::rect_pack::RectPacker packer = dp::rect_pack::RectPacker<int32_t>(maxPageSize.x, maxPageSize.y);

    std::vector<std::tuple<std::string, ::Vec2I>> sortedRectangles;

    for (const auto &[identifier, rectangle]: rectangles) {
        sortedRectangles.push_back({identifier, rectangle});
    }

    std::sort(sortedRectangles.begin(), sortedRectangles.end(), [](auto a, auto b) {
        const auto &[aId, aVec] = a;
        const auto &[bId, bVec] = b;
        if (aVec.y != bVec.y)
            return aVec.y > bVec.y;
        else
            return aVec.x > bVec.x;
    });

    for (const auto &[identifier, rectangle]: sortedRectangles) {
        const auto result = packer.insert(rectangle.x, rectangle.y);

        auto resultIt = results.find(result.pageIndex);
        if (resultIt == results.end()) {
            results.insert({ result.pageIndex, RectanglePackerPage({}) });
            resultIt = results.find(result.pageIndex);
        }

        resultIt->second.uvs.insert({identifier, RectI(result.pos.x, result.pos.y, rectangle.x, rectangle.y)});
    }

    std::vector<RectanglePackerPage> pages;
    for (const auto &[idx, page]: results) {
        pages.push_back(page);
    }

    return pages;

}
