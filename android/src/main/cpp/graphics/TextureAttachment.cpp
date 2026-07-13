/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "TextureAttachment.h"

#include "TextureHolderInterface.h"
#include <cassert>


#ifndef NDEBUG
TextureAttachment::~TextureAttachment() {
    assert(!isAttached() && "texture attachment not cleared clearFromGraphics");
}
#endif

bool TextureAttachment::isAttached() const {
    assert(textureHolder != nullptr || !texturePointer.has_value() && "texture can only be attached when texture holder is set");
    return texturePointer.has_value();
}

bool TextureAttachment::isSet() const {
    return textureHolder != nullptr;
}

GLuint TextureAttachment::texture() const  {
    return static_cast<GLuint>(texturePointer.value());
}

void TextureAttachment::set(std::shared_ptr<TextureHolderInterface> textureHolder) {
    if (this->textureHolder == textureHolder) {
        return;
    }

    clear();

    if (textureHolder) {
        _heightFactor = static_cast<float>(textureHolder->getImageHeight()) / static_cast<float>(textureHolder->getTextureHeight());
        _widthFactor = static_cast<float>(textureHolder->getImageWidth()) / static_cast<float>(textureHolder->getTextureWidth());
    }
    this->textureHolder = std::move(textureHolder);
}

bool TextureAttachment::attach(std::shared_ptr<TextureHolderInterface> textureHolder) {
    set(std::move(textureHolder));
    return attach();
}

bool TextureAttachment::attach() {
    if (textureHolder == nullptr || texturePointer.has_value()) {
        return false;
    }
    texturePointer = textureHolder->attachToGraphics();
    return true;
}

bool TextureAttachment::detach() {
    if (!texturePointer.has_value()) {
        return false;
    }
    assert(textureHolder != nullptr && "texture attached without texture holder?");
    textureHolder->clearFromGraphics();
    texturePointer.reset();
    return true;
}

bool TextureAttachment::clear() {
    const bool wasAttached = detach();
    textureHolder = nullptr;
    _heightFactor = 0.f;
    _widthFactor = 0.f;
    return wasAttached;
}
