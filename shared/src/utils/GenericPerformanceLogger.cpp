/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "GenericPerformanceLogger.h"
#include "ChronoUtil.h"
#include <cmath>

const size_t GenericPerformanceLogger::DEFAULT_NUM_BUCKETS = 100;
const int64_t GenericPerformanceLogger::DEFAULT_BUCKET_SIZE_MS = 5;

GenericPerformanceLogger::GenericPerformanceLogger(int32_t numBuckets, int64_t bucketSizeMs)
        : numBuckets(numBuckets), bucketSizeMs(bucketSizeMs) {}

std::optional<LoggerData> GenericPerformanceLogger::getStatistics(const std::string &id) {
    std::lock_guard<std::mutex> lock(mutex);
    auto logDataEntry = logData.find(id);
    if (logDataEntry != logData.end()) {
        logDataEntry->second->end = chronoutil::getCurrentTimestamp().count();
        return *logDataEntry->second;
    }
    return std::nullopt;
}

std::vector<LoggerData> GenericPerformanceLogger::getAllStatistics() {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<LoggerData> data;
    for (auto &[id, loggerData] : logData) {
        loggerData->end = chronoutil::getCurrentTimestamp().count();
        if (!loggerData) {
            continue;
        }
        data.push_back(*loggerData);
    }
    return data;
}

void GenericPerformanceLogger::ensureQuerySetup(const std::string &id) {
    auto logDataEntry = logData.find(id);
    if (logDataEntry == logData.end()) {
        logData[id] = std::make_shared<LoggerData>(
                id,
                std::vector<int64_t>(numBuckets, 0),
                bucketSizeMs,
                chronoutil::getCurrentTimestamp().count(),
                -1,
                0,
                0,
                0,
                0
        );
    }
}

void GenericPerformanceLogger::addTimeLog(const std::string &id, double deltaMs) {
    std::lock_guard<std::mutex> lock(mutex);

    // Update logger data
    auto loggerDataEntry = logData.find(id);
    auto loggerData = loggerDataEntry->second;
    if (loggerDataEntry == logData.end() || !loggerData) {
        return;
    }

    // Update histogram
    size_t bucketIndex = std::min((size_t) std::floor(deltaMs / bucketSizeMs), numBuckets - 1);
    loggerData->buckets[bucketIndex] += 1;

    // Update average
    int64_t prevNumSamples = loggerData->numSamples;
    if (prevNumSamples == 0) {
        loggerData->average = deltaMs;
        loggerData->variance = 0.0;
        loggerData->stdDeviation = 0.0;
    } else {
        double newAverage = (loggerData->average * loggerData->numSamples + deltaMs) / (prevNumSamples + 1);
        double newS = loggerData->variance * (prevNumSamples - 1) + // current S
                      (deltaMs - loggerData->average) * (deltaMs - newAverage);

        loggerData->average = newAverage;
        loggerData->variance = newS / prevNumSamples;
        loggerData->stdDeviation = sqrt(loggerData->variance);
    }

    // Update sample count
    loggerData->numSamples += 1;
}

void GenericPerformanceLogger::resetData() {
    std::lock_guard<std::mutex> lock(mutex);
    logData.clear();
}

void GenericPerformanceLogger::setLoggingEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex);
    this->enabled = enabled;
}