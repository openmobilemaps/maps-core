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

class AnimationInterface {
public:
    virtual void start() = 0;
    virtual void start(long long delay) = 0;
    virtual void cancel() = 0;
    virtual void finish() = 0;
    virtual bool isFinished() = 0;
    virtual void update() = 0;
};
