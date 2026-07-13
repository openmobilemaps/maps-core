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

#include "Coord.h"
#include "TrackedCoordinateCallbackInterface.h"
#include "TrackedCoordinateInterface.h"
#include "Vec2F.h"
#include <functional>
#include <memory>
#include <mutex>
#include <optional>

class TrackedCoordinate final : public TrackedCoordinateInterface {
  public:
    using Projector = std::function<std::optional<Vec2F>(const Coord &, bool)>;

    TrackedCoordinate(const Coord &coordinate,
                      const std::shared_ptr<TrackedCoordinateCallbackInterface> &callback,
                      Projector projector);

    void setCoordinate(const Coord &coordinate) override;

    void setVisible(bool visible) override;

    void notifyScreenPositionChanged();

  private:
    std::recursive_mutex mutex;
    Coord coordinate;
    bool visible = true;
    std::shared_ptr<TrackedCoordinateCallbackInterface> callback;
    Projector projector;
};
