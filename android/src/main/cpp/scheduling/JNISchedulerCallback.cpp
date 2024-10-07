/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "JNISchedulerCallback.h"
#include "JNISchedulerCallbackInterface.h"
#include "TaskPriority.h"
#include "Logger.h"

#include "sys/prctl.h"
#include "sys/resource.h"

#include "djinni_support.hpp"

JNISchedulerCallback::JNISchedulerCallback() {
    JNIEnv* env = djinni::jniGetThreadEnv();
    if (!env) {
        throw std::runtime_error("Failed to retrieve the environment of the scheduler creation thread!");
    }
    JavaVM* newVm = nullptr;
    env->GetJavaVM(&newVm);
    if (!newVm) {
        throw std::runtime_error("Failed to retrieve the JVM for the AndroidScheduler!");
    }
    vm = newVm;
}

std::string JNISchedulerCallback::getCurrentThreadName() {
    char name[32] = "";
    if (prctl(PR_GET_NAME, name) == -1) {
        LogError <<= "Couldn't get thread name";
    }
    return name;
}

void JNISchedulerCallback::setCurrentThreadName(const std::string &name) {
    if (prctl(PR_SET_NAME, name.c_str()) == -1) {
        LogError <<= "Couldn't set thread name: " + name;
    }
}

void JNISchedulerCallback::setThreadPriority(TaskPriority priority) {
    int p = 0;
    switch (priority) {
        case TaskPriority::HIGH:
            // THREAD_PRIORITY_FOREGROUND
            p = -2;
            break;
        case TaskPriority::NORMAL:
            // THREAD_PRIORITY_DEFAULT
            p = 0;
            break;
        case TaskPriority::LOW:
            // THREAD_PRIORITY_BACKGROUND
            p = 10;
            break;
    }
    setpriority(PRIO_PROCESS, 0, p);
}

void JNISchedulerCallback::attachThread() {
    if (!vm) {
        throw std::runtime_error("Invalid JVM on attaching thread!");
    }
    JNIEnv *env = nullptr;
    jint envRes = vm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (envRes != JNI_OK) {
        if (envRes == JNI_EDETACHED) {

#if defined(ANDROID) || defined(__ANDROID__)
            jint attachRes = vm->AttachCurrentThread(&env, nullptr);
#else
            jint attachRes = vm->AttachCurrentThread(reinterpret_cast<void**>(&env), nullptr);
#endif
            if (attachRes != JNI_OK) {
                throw std::runtime_error("Failed to attach thread to JVM!");
            }
        } else {
            throw std::runtime_error("Failed to determine the JNIEnv of the current thread!");
        }
    }
}

void JNISchedulerCallback::detachThread() {
    if (!vm) {
        throw std::runtime_error("Invalid JVM on detaching thread!");
    }
    JNIEnv *env = nullptr;
    jint envRes = vm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (envRes == JNI_OK) {
        jint detachRes = vm->DetachCurrentThread();
        if (detachRes != JNI_OK) {
            throw std::runtime_error("Failed to detach thread from JVM!");
        }
    }
}
