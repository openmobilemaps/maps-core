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

#include "CpuPerformanceLoggerInterface.h"
#include "GenericPerformanceLogger.h"

class CpuPerformanceLogger
        : public CpuPerformanceLoggerInterface,
          public GenericPerformanceLogger,
          public std::enable_shared_from_this<CpuPerformanceLogger> {
public:
    CpuPerformanceLogger();

    CpuPerformanceLogger(int32_t numBuckets, int64_t bucketSizeMs);

    std::shared_ptr<PerformanceLoggerInterface> asPerformanceLoggerInterface() override;

    void startLog(const std::string &id) override;

    void endLog(const std::string &id) override;

    void resetData() override;

    std::string getLoggerName() override;

private:
    std::unordered_map<std::string, std::chrono::nanoseconds> queryStartTimestamps;
};
