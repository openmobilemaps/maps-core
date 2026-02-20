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

const bool BaseGraphicsObjectOpenGl::clearOnPause = false;

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

