/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "CpuPerformanceLogger.h"
#include "ChronoUtil.h"

CpuPerformanceLogger::CpuPerformanceLogger() : GenericPerformanceLogger() {}

CpuPerformanceLogger::CpuPerformanceLogger(int32_t numBuckets, int64_t bucketSizeMs) : GenericPerformanceLogger(numBuckets, bucketSizeMs) {}

std::shared_ptr<PerformanceLoggerInterface> CpuPerformanceLogger::asPerformanceLoggerInterface() {
    return shared_from_this();
}

void CpuPerformanceLogger::startLog(const std::string &id) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!enabled) {
        return;
    }

    ensureQuerySetup(id);

    queryStartTimestamps[id] = chronoutil::getCurrentTimestampNs();
}

void CpuPerformanceLogger::endLog(const std::string &id) {
    double deltaMs = -1;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (!enabled) {
            return;
        }

        std::chrono::nanoseconds endTimestamp = chronoutil::getCurrentTimestampNs();
        const auto startTimestamp = queryStartTimestamps.find(id);
        if (startTimestamp != queryStartTimestamps.end()) {
            deltaMs = (endTimestamp - startTimestamp->second).count() / 1000000.0;
        }
    }

    if (deltaMs >= 0) {
        addTimeLog(id, deltaMs);
    }
}

void CpuPerformanceLogger::resetData() {
    GenericPerformanceLogger::resetData();
    {
        std::lock_guard<std::mutex> lock(mutex);
        queryStartTimestamps.clear();
    }
}

std::string CpuPerformanceLogger::getLoggerName() {
    return "CpuPerformanceLogger";
}