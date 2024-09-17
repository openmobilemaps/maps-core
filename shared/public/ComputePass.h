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

#include <unordered_map>
#include "ComputeObjectInterface.h"
#include "ComputePassInterface.h"

class ComputePass : public ComputePassInterface {
public:
    ComputePass(const std::shared_ptr<::ComputeObjectInterface> &computeObject);

    ComputePass(const std::vector<std::shared_ptr<::ComputeObjectInterface>> &computeObjects);

    virtual std::vector<std::shared_ptr<::ComputeObjectInterface>> getComputeObjects() override;

private:
    std::vector<std::shared_ptr<::ComputeObjectInterface>> computeObjects;
};
