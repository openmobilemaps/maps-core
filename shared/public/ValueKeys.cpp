/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "ValueKeys.h"


const char *ValueKeys::IDENTIFIER_KEY_STR = "identifier";
const char *ValueKeys::TYPE_KEY_STR = "$type";

static StringInterner staticStringTable{};
const InternedString ValueKeys::IDENTIFIER_KEY = staticStringTable.add(ValueKeys::IDENTIFIER_KEY_STR);
const InternedString ValueKeys::TYPE_KEY = staticStringTable.add(ValueKeys::TYPE_KEY_STR);
const InternedString ValueKeys::ZOOM = staticStringTable.add("zoom");

StringInterner ValueKeys::newStringInterner() {
    return StringInterner(staticStringTable);
}
