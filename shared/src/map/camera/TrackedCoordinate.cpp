/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "TrackedCoordinate.h"

TrackedCoordinate::TrackedCoordinate(const Coord &coordinate,
                                     const std::shared_ptr<TrackedCoordinateCallbackInterface> &callback,
                                     Projector projector)
    : coordinate(coordinate)
    , callback(callback)
    , projector(std::move(projector)) {}

void TrackedCoordinate::setCoordinate(const Coord &coordinate) {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        this->coordinate = coordinate;
    }
    notifyScreenPositionChanged();
}

void TrackedCoordinate::setVisible(bool visible) {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        this->visible = visible;
    }
    notifyScreenPositionChanged();
}

void TrackedCoordinate::notifyScreenPositionChanged() {
    Coord coordinate = this->coordinate;
    bool visible = this->visible;
    std::shared_ptr<TrackedCoordinateCallbackInterface> callback = this->callback;
    Projector projector = this->projector;

    {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        coordinate = this->coordinate;
        visible = this->visible;
        callback = this->callback;
        projector = this->projector;
    }

    if (!callback || !projector) {
        return;
    }

    callback->screenPositionDidChange(projector(coordinate, visible));
}
