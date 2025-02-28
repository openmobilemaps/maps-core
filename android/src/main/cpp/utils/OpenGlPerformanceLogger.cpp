/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "OpenGlPerformanceLogger.h"

OpenGlPerformanceLogger::OpenGlPerformanceLogger() : GenericPerformanceLogger() {}

OpenGlPerformanceLogger::OpenGlPerformanceLogger(int32_t numBuckets, int64_t bucketSizeMs) : GenericPerformanceLogger(numBuckets,
                                                                                                                      bucketSizeMs) {}

std::shared_ptr<PerformanceLoggerInterface> OpenGlPerformanceLogger::asPerformanceLoggerInterface() {
    return shared_from_this();
}

void OpenGlPerformanceLogger::startLog(const std::string &id) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!enabled) {
        return;
    }

    if (queryId == 0) {
        GLuint newQueryId[1];
        glGenQueries(1, newQueryId);
        queryId = newQueryId[0];
    }

    ensureQuerySetup(id);

    glBeginQuery(0x88BF, queryId);
    OpenGlHelper::checkGlError("OpenGlPerformanceLogger::startLog");
}

void OpenGlPerformanceLogger::endLog(const std::string &id) {
    double deltaMs = -1;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (!enabled) {
            return;
        }

        glEndQuery(0x88BF);
        GLuint timestamps[1];
        glGetQueryObjectuiv(queryId, GL_QUERY_RESULT, &timestamps[0]);

        deltaMs = timestamps[0] / 1000000.0;
    }

    OpenGlHelper::checkGlError("OpenGlPerformanceLogger::endLog");

    if (deltaMs >= 0) {
        addTimeLog(id, deltaMs);
    }
}

void OpenGlPerformanceLogger::resetData() {
    GenericPerformanceLogger::resetData();
    {
        std::lock_guard<std::mutex> lock(mutex);
        GLuint queryIdToDelete[1];
        if (queryId != 0) {
            queryIdToDelete[0] = queryId;
            glDeleteQueries(1, queryIdToDelete);
        }
        queryId = 0;
    }
}

std::string OpenGlPerformanceLogger::getLoggerName() {
    return "OpenGlPerformanceLogger";
}
