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
        guard let touchHandler else { return }
        touches.forEach {
            activeTouches.insert($0)
            originalTouchLocations[$0] = $0.location(in: mapView)
            touchHandler.onTouchEvent(activeTouches.asMCTouchEvent(in: mapView, scale: Float(mapView.contentScaleFactor), action: .DOWN))
        }
    }

    private func touchUp(_ touches: Set<UITouch>) {
        guard let touchHandler else { return }
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
        guard let touchHandler else { return }
        touches.forEach {
            activeTouches.insert($0)

            touchHandler.onTouchEvent(activeTouches.asMCTouchEvent(in: mapView, scale: Float(mapView.contentScaleFactor), action: .CANCEL))

            activeTouches.remove($0)
            originalTouchLocations.removeValue(forKey: $0)
        }
    }

    func touchesMoved(_ touches: Set<UITouch>, with _: UIEvent?) {
        guard let touchHandler else { return }

        touches.forEach {
            activeTouches.insert($0)
            touchHandler.onTouchEvent(activeTouches.asMCTouchEvent(in: mapView, scale: Float(mapView.contentScaleFactor), action: .MOVE))
        }
    }

    func cancelAllTouches() {
        touchUp(activeTouches)
    }

    func handlePan(panGestureRecognizer: UIPanGestureRecognizer) {
        guard let touchHandler, let mapView else { return }

        let contentScale = Float(mapView.contentScaleFactor)
        let touches = [panGestureRecognizer.location(in: mapView)]

        switch panGestureRecognizer.state {
            case .began:
                let te = touches.asMCTouchEvent(scale: contentScale, action: .DOWN)
                touchHandler.onTouchEvent(te)
            case .changed:
                let te = touches.asMCTouchEvent(scale: contentScale, action: .MOVE)
                touchHandler.onTouchEvent(te)
            case .ended:
                let te = touches.asMCTouchEvent(scale: contentScale, action: .UP)
                touchHandler.onTouchEvent(te)
            case .failed, .cancelled:
                let te = touches.asMCTouchEvent(scale: contentScale, action: .CANCEL)
                touchHandler.onTouchEvent(te)
            default:
                break
        }
    }

    func handlePinch(pinchGestureRecognizer: UIPinchGestureRecognizer) {
        guard let touchHandler, let mapView else { return }

        let contentScale = Float(mapView.contentScaleFactor)

        let magicConstant = 1.414 // makes feel like using iOS app natively
        let s = pow(pinchGestureRecognizer.scale, magicConstant)
        let touch = pinchGestureRecognizer.location(in: mapView)
        let halfWidth = 100.0

        switch pinchGestureRecognizer.state {
            case .began:
                let t0 = CGPoint(x: touch.x - halfWidth, y: touch.y)
                let t1 = CGPoint(x: touch.x + halfWidth, y: touch.y)
                let touches = [t0, t1]

                let te = touches.asMCTouchEvent(scale: contentScale, action: .DOWN)
                touchHandler.onTouchEvent(te)
            case .changed:
                let t0 = CGPoint(x: touch.x - halfWidth * s, y: touch.y)
                let t1 = CGPoint(x: touch.x + halfWidth * s, y: touch.y)
                let touches = [t0, t1]

                let te = touches.asMCTouchEvent(scale: contentScale, action: .MOVE)
                touchHandler.onTouchEvent(te)
            case .ended:
                let t0 = CGPoint(x: touch.x - halfWidth * s, y: touch.y)
                let t1 = CGPoint(x: touch.x + halfWidth * s, y: touch.y)
                let touches = [t0, t1]

                let te = touches.asMCTouchEvent(scale: contentScale, action: .UP)
                touchHandler.onTouchEvent(te)
            case .failed, .cancelled:
                let t0 = CGPoint(x: touch.x - halfWidth * s, y: touch.y)
                let t1 = CGPoint(x: touch.x + halfWidth * s, y: touch.y)
                let touches = [t0, t1]

                let te = touches.asMCTouchEvent(scale: contentScale, action: .CANCEL)
                touchHandler.onTouchEvent(te)
            default:
                break
        }
    }
}

private extension Set<UITouch> {
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

private extension [CGPoint] {
    func asMCTouchLocation(scale: Float) -> [MCVec2F] {
        map {
            let x = Float($0.x) * scale
            let y = Float($0.y) * scale
            return MCVec2F(x: x, y: y)
        }
    }

    func asMCTouchEvent(scale: Float, action: MCTouchAction) -> MCTouchEvent {
        MCTouchEvent(pointers: asMCTouchLocation(scale: scale), touchAction: action)
    }
}
