/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import MapCoreSharedModule
import UIKit

open class MCMapViewTouchHandler: NSObject {
    private var touchHandler: MCTouchHandlerInterface?

    weak var mapView: UIView!

    private var activeTouches = Set<UITouch>()

    private var originalTouchLocations: [UITouch: CGPoint] = [:]

    init(touchHandler: MCTouchHandlerInterface?) {
        self.touchHandler = touchHandler
        super.init()
    }

    func touchesBegan(_ touches: Set<UITouch>, with _: UIEvent?) {
        guard let touchHandler = touchHandler else { return }
        touches.forEach {
            activeTouches.insert($0)
            originalTouchLocations[$0] = $0.location(in: mapView)
            touchHandler.onTouchEvent(activeTouches.asMCTouchEvent(in: mapView, scale: Float(mapView.contentScaleFactor), action: .DOWN))
        }
    }

    private func touchUp(_ touches: Set<UITouch>) {
        guard let touchHandler = touchHandler else { return }
        touches.forEach {
            activeTouches.insert($0)

            touchHandler.onTouchEvent(activeTouches.asMCTouchEvent(in: mapView, scale: Float(mapView.contentScaleFactor), action: .UP))

            activeTouches.remove($0)
            originalTouchLocations.removeValue(forKey: $0)
        }
    }

    func touchesEnded(_ touches: Set<UITouch>, with _: UIEvent?) {
        touchUp(touches)
    }

    func touchesCancelled(_ touches: Set<UITouch>, with _: UIEvent?) {
        guard let touchHandler = touchHandler else { return }
        touches.forEach {
            activeTouches.insert($0)

            touchHandler.onTouchEvent(activeTouches.asMCTouchEvent(in: mapView, scale: Float(mapView.contentScaleFactor), action: .CANCEL))

            activeTouches.remove($0)
            originalTouchLocations.removeValue(forKey: $0)
        }
    }

    func touchesMoved(_ touches: Set<UITouch>, with _: UIEvent?) {
        guard let touchHandler = touchHandler else { return }

        func CGPointDistanceSquared(from: CGPoint, to: CGPoint) -> CGFloat {
            (from.x - to.x) * (from.x - to.x) + (from.y - to.y) * (from.y - to.y)
        }

        func CGPointDistance(from: CGPoint, to: CGPoint) -> CGFloat {
            sqrt(CGPointDistanceSquared(from: from, to: to))
        }

        touches.forEach {
            activeTouches.insert($0)
            touchHandler.onTouchEvent(activeTouches.asMCTouchEvent(in: mapView, scale: Float(mapView.contentScaleFactor), action: .MOVE))
        }
    }

    func cancelAllTouches() {
        touchUp(activeTouches)
    }
}

private extension Set where Element == UITouch {
    func asMCTouchLocation(in view: UIView, scale: Float) -> [MCVec2F] {
        map {
            let location = $0.location(in: view)
            let x = Float(location.x) * scale
            let y = Float(location.y) * scale
            return MCVec2F(x: x, y: y)
        }
    }

    func asMCTouchEvent(in view: UIView, scale: Float, action: MCTouchAction) -> MCTouchEvent {
        MCTouchEvent(pointers: asMCTouchLocation(in: view, scale: scale), touchAction: action)
    }
}
