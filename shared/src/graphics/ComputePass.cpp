/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "ComputePass.h"

ComputePass::ComputePass(const std::shared_ptr<::ComputeObjectInterface> &computeObject)
    : computeObjects({computeObject}) {}

ComputePass::ComputePass(const std::vector<std::shared_ptr<::ComputeObjectInterface>> &computeObjects)
    : computeObjects(computeObjects) {}

std::vector<std::shared_ptr<::ComputeObjectInterface>> ComputePass::getComputeObjects() { return computeObjects; }
