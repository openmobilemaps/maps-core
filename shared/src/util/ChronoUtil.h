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

#include <Logger.h>
#include <chrono>
#include <string>

namespace chronoutil {
    static std::chrono::milliseconds getCurrentTimestamp() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    }

    static std::string getTimestampDeltaString(std::chrono::milliseconds start, std::chrono::milliseconds end) {
        return std::to_string((end - start).count()) + "ms";
    }

    static void logDebugTimeDeltaToNow(std::string tag, std::chrono::milliseconds start) {
        LogDebug <<= tag + ": " + getTimestampDeltaString(start, getCurrentTimestamp());
    }
}
