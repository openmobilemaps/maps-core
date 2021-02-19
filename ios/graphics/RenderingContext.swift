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
import Metal

@objc
class RenderingContext: NSObject {
    weak var encoder: MTLRenderCommandEncoder?
    weak var sceneView: MCMapView?

    var viewportSize: MCVec2I = .init(x: 0, y: 0)
}

extension RenderingContext: MCRenderingContextInterface {
    func setupDrawFrame() {}

    func onSurfaceCreated() {}

    func setViewportSize(_ newSize: MCVec2I) {
        viewportSize = newSize
    }

    func getViewportSize() -> MCVec2I { viewportSize }

    func setBackgroundColor(_ color: MCColor) {
        sceneView?.clearColor = color.metalColor
    }
}
