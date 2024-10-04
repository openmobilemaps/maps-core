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

#include "ThreadPoolCallbacks.h"

#include "jni.h"

class JNISchedulerCallback : public ThreadPoolCallbacks {
public:
    JNISchedulerCallback();

    ~JNISchedulerCallback() = default;

    std::string getCurrentThreadName() override;

    void setCurrentThreadName(const std::string &name) override;

    void setThreadPriority(TaskPriority priority) override;

    void attachThread() override;

    void detachThread() override;

private:
    JavaVM* vm = nullptr;
};
