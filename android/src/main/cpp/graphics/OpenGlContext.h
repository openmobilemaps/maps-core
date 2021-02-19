/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#ifndef MAPSDK_OPENGLCONTEXT_H
#define MAPSDK_OPENGLCONTEXT_H

#include "RenderingContextInterface.h"
#include <map>
#include <string>
#include <vector>

class OpenGlContext : public RenderingContextInterface, std::enable_shared_from_this<OpenGlContext> {
  public:
    OpenGlContext();

    int getProgram(std::string name);

    void storeProgram(std::string name, int program);

    std::vector<unsigned int> &getTexturePointerArray(std::string name, int capacity);

    void cleanAll();

    virtual void onSurfaceCreated() override;

    virtual void setViewportSize(const ::Vec2I &size) override;

    virtual ::Vec2I getViewportSize() override;

    virtual void setBackgroundColor(const ::Color &color) override;

    virtual void setupDrawFrame() override;

  protected:
    Color backgroundColor = Color(0, 0, 0, 1);

    std::map<std::string, int> programs;
    std::map<std::string, std::vector<unsigned int>> texturePointers;

    Vec2I viewportSize = Vec2I(0, 0);
};

#endif // MAPSDK_OPENGLCONTEXT_H
