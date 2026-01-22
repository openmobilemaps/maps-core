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

#define POLYGON_SUBDIVISION_FACTOR 10.0
#define POLYGON_MASK_SUBDIVISION_FACTOR 10.0

#if defined(__ANDROID__)
    #define HARDWARE_TESSELLATION_SUPPORTED
#endif

#if defined(__APPLE__)
    #include <TargetConditionals.h>
    #if !TARGET_OS_SIMULATOR
        #define HARDWARE_TESSELLATION_SUPPORTED
    #endif
#endif

#define HARDWARE_TESSELLATION_WIREFRAME 0
