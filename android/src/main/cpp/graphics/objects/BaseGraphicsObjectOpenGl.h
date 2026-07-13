/*
 * Copyright (c) 2026 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "GraphicsObjectInterface.h"

class BaseGraphicsObjectOpenGl : public GraphicsObjectInterface {
public:
    virtual ~BaseGraphicsObjectOpenGl() = default;

    virtual void pause() override;
    virtual void resume(const std::shared_ptr<RenderingContextInterface> &context) override;

protected:
    static void enableDepthTest();
    static void disableDepthTest();

    static const bool clearOnPause;
};
