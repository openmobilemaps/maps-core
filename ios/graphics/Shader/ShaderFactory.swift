/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import Foundation
import MapCoreSharedModule

class ShaderFactory: MCShaderFactoryInterface {
    func createAlphaShader() -> MCAlphaShaderInterface? {
        AlphaShader()
    }

    func createColorLineShader() -> MCColorLineShaderInterface? {
        ColorLineShader()
    }

    func createColorShader() -> MCColorShaderInterface? {
        ColorShader()
    }
}
