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

#include "LineInfoInterface.h"

namespace std {
template <> struct hash<std::shared_ptr<LineInfoInterface>> {
    inline size_t operator()(const std::shared_ptr<LineInfoInterface> &obj) const { return std::hash<std::string>{}(obj->getIdentifier()); }
};
template <> struct equal_to<std::shared_ptr<LineInfoInterface>> {
    inline bool operator()(const std::shared_ptr<LineInfoInterface> &lhs, const std::shared_ptr<LineInfoInterface> &rhs) const { return lhs->getIdentifier() == rhs->getIdentifier(); }
};
}; // namespace std
