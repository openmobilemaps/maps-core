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

#include "InternedString.h"
#include "StringInterner.h"

class ValueKeys {
public:
    const static char *IDENTIFIER_KEY_STR;
    const static char *TYPE_KEY_STR;

    const static InternedString IDENTIFIER_KEY;
    const static InternedString TYPE_KEY;
    const static InternedString ZOOM;

    // Create a StringInterner containing the pre-defined values above
    static StringInterner newStringInterner();
};
