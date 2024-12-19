/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Logger.h"
#include <cstdarg>
#include <iostream>

#ifdef _WIN32
#include "WindowsLogger.h"
#endif

using namespace utility;

#ifdef __ANDROID__
extern "C" {
int __android_log_print(int prio, const char *tag, const char *fmt, ...);
}
#endif

#if defined(__APPLE__) && !defined(BANDIT_TESTING)
#include <os/log.h>
#endif

//#define SHOW_COLORS 0

#ifndef LOG_LEVEL
#define LOG_LEVEL 2 // == LogDebug
#endif

Logger::Logger(int p){
    priority = p;
}

std::stringstream &Logger::stream() const { return ss; }

void Logger::log(int prio, const char *tag, const char *fmt, ...) const {
#ifdef __ANDROID__
    int androidPrio = 3;
    switch (priority) {
    case 0: {
        androidPrio = 6;
        break;
    }
    case 1: {
        androidPrio = 5;
        break;
    }
    case 2: {
        androidPrio = 3;
        break;
    }
    case 3: {
        androidPrio = 4;
        break;
    }
    case 4: {
        androidPrio = 2;
        break;
    }
    default: {
        break;
    }
    }

    va_list args;
    va_start(args, fmt);
    __android_log_print(androidPrio, tag, fmt, args);
    va_end(args);
#endif

#if (defined(__APPLE__) && !defined(BANDIT_TESTING)) || defined(_WIN32)
    switch (priority) {
    case 0: {
        os_log(OS_LOG_DEFAULT, "%{public}s", fmt);
        break;
    }
    case 1: {
        os_log_error(OS_LOG_DEFAULT, "%{public}s", fmt);
        break;
    }
    case 2: {
        os_log_debug(OS_LOG_DEFAULT, "%{public}s", fmt);
        break;
    }
    case 3: {
        os_log_info(OS_LOG_DEFAULT, "%{public}s", fmt);
        break;
    }
    case 4: {
        os_log(OS_LOG_DEFAULT, "%{public}s", fmt);
        break;
    }
    default: {
        os_log(OS_LOG_DEFAULT, "%{PUBLIC}s", fmt);
    }
    }
#endif

#if defined(BANDIT_TESTING) || (defined(__linux__) && !defined(__ANDROID__))
    if (priority <= LOG_LEVEL) {
        switch (priority) {
        case 0: {
            std::cout
#ifdef SHOW_COLORS
                << "\033["
                << "fg192,53,40;"
#endif
                << "ERROR: " << fmt << std::endl
#ifdef SHOW_COLORS
                << "\033[;"
#endif
                ;
            break;
        }
        case 1: {
            std::cout
#ifdef SHOW_COLORS
                << "\033["
                << "fg211,84,0;"
#endif
                << "WARNING: " << fmt << std::endl
#ifdef SHOW_COLORS
                << "\033[;"
#endif
                ;
            break;
        }
        case 2: {
            std::cout
#ifdef SHOW_COLORS
                << "\033["
                << "fg22,160,133;"
#endif
                << "DEBUG: " << fmt << std::endl
#ifdef SHOW_COLORS
                << "\033[;"
#endif
                ;
            break;
        }
        case 3: {
            std::cout
#ifdef SHOW_COLORS
                << "\033["
                << "fg41,128,185;"
#endif
                << "INFO: " << fmt << std::endl
#ifdef SHOW_COLORS
                << "\033[;"
#endif
                ;
            break;
        }
        case 4: {
            std::cout
#ifdef SHOW_COLORS
                << "\033["
                << "fg127,140,141;"
#endif
                << "TRACE: " << fmt << std::endl
#ifdef SHOW_COLORS
                << "\033[;"
#endif
                ;
            break;
        }
        default: {
            break;
        }
        }
    }
#endif
}
