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

#include "PerformanceLoggerInterface.h"
#include "LoggerData.h"
#include "unordered_map"

class GenericPerformanceLogger : public PerformanceLoggerInterface {
public:
    GenericPerformanceLogger() {};

    GenericPerformanceLogger(int32_t numBuckets, int64_t bucketSizeMs);

    std::optional<LoggerData> getStatistics(const std::string &id) override;

    std::vector<LoggerData> getAllStatistics() override;

    void resetData() override;

    void setLoggingEnabled(bool enabled) override;

    const static size_t DEFAULT_NUM_BUCKETS;
    const static int64_t DEFAULT_BUCKET_SIZE_MS;

protected:
    void ensureQuerySetup(const std::string &id);

    void addTimeLog(const std::string &id, double deltaMs);

    size_t numBuckets = DEFAULT_NUM_BUCKETS;
    size_t bucketSizeMs = DEFAULT_BUCKET_SIZE_MS;

    std::mutex mutex;
    bool enabled = true;
    std::unordered_map<std::string, std::shared_ptr<LoggerData>> logData;
};
