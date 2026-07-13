
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

#include "opengl_wrapper.h"
#include <memory>
#include <optional>
#include <cstdint>

class TextureHolderInterface;

/**
 * TextureAttachment is a simple helper for TextureHolderInterface's
 * attachToGraphics/clearFromGraphics.
 *
 * All operations, set/attach/detach/clear, must be called with a GL context
 * (on the graphics thread).
 */
class TextureAttachment {
private:
    std::shared_ptr<TextureHolderInterface> textureHolder = nullptr;
    std::optional<int32_t> texturePointer;
    float _heightFactor = 0.f;
    float _widthFactor = 0.f;

public:
#ifndef NDEBUG
    ~TextureAttachment();
#endif

    /**
     * @returns true iff texture is attached. Implies isSet.
     */
    bool isAttached() const;
    /**
     * @returns true iff non-null texture was set (via set(textureHolder) or attach(textureHolder)).
     */
    bool isSet() const;


    /**
     * @returns the texture "name" returned by textureHolder->attachToGraphics()
     * @pre isAttached();
     */
    GLuint texture() const;
    /**
     * @returns imageHeight / textureHeight
     * @pre isSet()
     */
    float heightFactor() const { return _heightFactor; };
    /**
     * @returns imageWidth / textureWidth
     * @pre isSet()
     */
    float widthFactor() const { return _widthFactor; };


    /**
     * Set the texture. Clears previous attachment, unless identical to new textureHolder.
     */
    void set(std::shared_ptr<TextureHolderInterface> textureHolder);
    /**
     * Set and attach the texture. Clears previous attachment if any.
     * @returns true iff newly attached or attachment changed
     */
    bool attach(std::shared_ptr<TextureHolderInterface> textureHolder);
    /**
     * Detach the texture.
     * @returns true iff newly attached
     */
    bool attach();
    /**
     * Detach  the texture (`textureHolder->clearFromGraphics()`).
     * @returns true iff was attached before
     */
    bool detach();
    /**
     * Detach and unset the texture holder.
     * @returns true iff was attached
     */
    bool clear();
};
