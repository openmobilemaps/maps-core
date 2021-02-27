/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#ifndef MAPSDKEXAMPLE_ICONINFO_H
#define MAPSDKEXAMPLE_ICONINFO_H


#include "IconInfoInterface.h"

class IconInfo : public IconInfoInterface {
public:
    IconInfo(const std::string & identifier, const ::Coord & coordinate, const std::shared_ptr<::TextureHolderInterface> & texture, const ::Vec2F & iconSize, IconType type);

    virtual ~IconInfo() {}

    virtual std::string getIdentifier() override;

    virtual std::shared_ptr<::TextureHolderInterface> getTexture() override;

    virtual void setCoordinate(const ::Coord & coord) override;

    virtual ::Coord getCoordinate() override;

    virtual void setIconSize(const ::Vec2F & size) override;

    virtual ::Vec2F getIconSize() override;

    virtual void setType(IconType type) override;

    virtual IconType getType() override;

private:
    std::string identifier;
    Coord coordinate;
    std::shared_ptr<::TextureHolderInterface> texture;
    Vec2F iconSize;
    IconType type;
};


#endif //MAPSDKEXAMPLE_ICONINFO_H
