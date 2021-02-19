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

#include "TaskConfig.h"
#include "TaskInterface.h"
#include <functional>

class LambdaTask : public TaskInterface {
  public:
    LambdaTask(TaskConfig config_, std::function<void()> method_);
    virtual TaskConfig getConfig() override;
    virtual void run() override;

  private:
    TaskConfig config;
    std::function<void()> method;
};
