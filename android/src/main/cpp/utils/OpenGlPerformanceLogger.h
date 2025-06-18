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

#include "OpenGlPerformanceLoggerInterface.h"
#include "OpenGlHelper.h"
#include "GenericPerformanceLogger.h"

class OpenGlPerformanceLogger : public OpenGlPerformanceLoggerInterface, public GenericPerformanceLogger, public std::enable_shared_from_this<OpenGlPerformanceLogger> {
public:
    OpenGlPerformanceLogger();

    OpenGlPerformanceLogger(int32_t numBuckets, int64_t bucketSizeMs);

    std::shared_ptr<PerformanceLoggerInterface> asPerformanceLoggerInterface() override;

    // Ensure that different logs are not interleaving! Not supported for OpenGL time querying
    void startLog(const std::string &id) override;

    void endLog(const std::string &id) override;

    void resetData() override;

    std::string getLoggerName() override;

private:
    GLuint queryId = 0;
};