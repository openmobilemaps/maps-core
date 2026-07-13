/*
 * Copyright (c) 2026 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "BaseGraphicsObjectOpenGl.h"
#include "RenderingContextInterface.h"
#include "opengl_wrapper.h"

#ifndef OPENMOBILEMAPS_GL_CLEAR_ON_PAUSE
#ifdef __ANDROID__
#define OPENMOBILEMAPS_GL_CLEAR_ON_PAUSE true
#else
#define OPENMOBILEMAPS_GL_CLEAR_ON_PAUSE false
#endif
#endif

const bool BaseGraphicsObjectOpenGl::clearOnPause = OPENMOBILEMAPS_GL_CLEAR_ON_PAUSE;

void BaseGraphicsObjectOpenGl::pause() {
    if(clearOnPause) {
        clear();
    }
}

void BaseGraphicsObjectOpenGl::resume(const std::shared_ptr<RenderingContextInterface> &context) {
    if(clearOnPause) {
        setup(context);
    }
}

void BaseGraphicsObjectOpenGl::enableDepthTest() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
}

void BaseGraphicsObjectOpenGl::disableDepthTest() {
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
}
