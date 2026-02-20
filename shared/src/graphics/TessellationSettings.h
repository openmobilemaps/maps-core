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

#define POLYGON_SUBDIVISION_FACTOR 4
#define POLYGON_MASK_SUBDIVISION_FACTOR 4

#ifndef HARDWARE_TESSELLATION_SUPPORTED
    #if defined(__ANDROID__)
        #define HARDWARE_TESSELLATION_SUPPORTED 1
    #elif defined(__APPLE__)
        #include <TargetConditionals.h>
        #define HARDWARE_TESSELLATION_SUPPORTED !TARGET_OS_SIMULATOR
    #else
        #define HARDWARE_TESSELLATION_SUPPORTED 0
    #endif
#endif

// OpenGL wireframe overlay (set to 1 to enable).
#ifndef HARDWARE_TESSELLATION_WIREFRAME_OPENGL
    #define HARDWARE_TESSELLATION_WIREFRAME_OPENGL 0
#endif
// Metal wireframe overlay is controlled by Swift build settings in:
// maps-core/Package.swift (swiftSettings). Define HARDWARE_TESSELLATION_WIREFRAME_METAL.
