/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "OpenGlPerformanceLoggerInterface.h"
#include "OpenGlPerformanceLogger.h"

std::shared_ptr<OpenGlPerformanceLoggerInterface> OpenGlPerformanceLoggerInterface::create() {
    return std::make_shared<OpenGlPerformanceLogger>();
}

std::shared_ptr<OpenGlPerformanceLoggerInterface> OpenGlPerformanceLoggerInterface::createSpecifically(int32_t numBuckets,
                                                                                                       int64_t bucketSizeMs) {
    return std::make_shared<OpenGlPerformanceLogger>(numBuckets, bucketSizeMs);
}