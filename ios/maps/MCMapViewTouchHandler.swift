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
@preconcurrency import MetalKit

#if canImport(UIKit)
    import UIKit
#elseif canImport(AppKit) && !canImport(UIKit)
    import AppKit
#endif

@MainActor
open class MCMapViewTouchHandler: NSObject {
    private var touchHandler: MCTouchHandlerInterface?

    weak var mapView: MCPlatformView!

    #if canImport(UIKit)
        private var activeTouches = Set<UITouch>()
        private var isHovering = false
        private var originalTouchLocations: [UITouch: CGPoint] = [:]
    #endif

    init(touchHandler: MCTouchHandlerInterface?) {
        self.touchHandler = touchHandler
        super.init()
    }

    #if canImport(UIKit)
        func touchesBegan(_ touches: Set<UITouch>, with _: UIEvent?) {
            guard let touchHandler else { return }
            touches.forEach {
                activeTouches.insert($0)
                isHovering = false
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
            cancelHover()
        }

        func cancelHover() {
            guard let touchHandler, isHovering else { return }
            touchHandler.onTouchEvent(MCTouchEvent(pointers: [], scrollDelta: 0, touchAction: .HOVER_EXIT))
            isHovering = false
        }

        func handlePan(panGestureRecognizer: UIPanGestureRecognizer) {
            guard let touchHandler, let mapView else { return }

            let contentScale = Float(mapView.contentScaleFactor)
            let touches = [panGestureRecognizer.location(in: mapView)]

            switch panGestureRecognizer.state {
                case .began:
                    isHovering = false
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

            let magicConstant = 2.414  // makes feel like using iOS app natively
            let s = pow(pinchGestureRecognizer.scale, magicConstant)
            let touch = pinchGestureRecognizer.location(in: mapView)
            let halfWidth = 100.0

            switch pinchGestureRecognizer.state {
                case .began:
                    isHovering = false
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

        @available(iOS 13.4, *)
        func handleHover(hoverGestureRecognizer: UIHoverGestureRecognizer) {
            guard let touchHandler, let mapView, activeTouches.isEmpty else { return }

            let contentScale = Float(mapView.contentScaleFactor)
            let touches = [hoverGestureRecognizer.location(in: mapView)]

            switch hoverGestureRecognizer.state {
                case .began, .changed:
                    isHovering = true
                    let te = touches.asMCTouchEvent(scale: contentScale, action: .HOVER)
                    touchHandler.onTouchEvent(te)
                case .ended, .cancelled, .failed:
                    cancelHover()
                default:
                    break
            }
        }
    #elseif canImport(AppKit) && !canImport(UIKit)
        func cancelAllTouches() {
            // No touch tracking state on macOS gesture recognizers.
        }

        private func normalizedMapPoint(_ point: CGPoint, in mapView: MCPlatformView) -> CGPoint {
            CGPoint(x: point.x, y: mapView.bounds.height - point.y)
        }

        func handlePan(panGestureRecognizer: NSPanGestureRecognizer) {
            guard let touchHandler, let mapView else { return }

            let contentScale = Float(MCDisplayMetrics.nativeScale(for: mapView as? MTKView))
            let touches = [normalizedMapPoint(panGestureRecognizer.location(in: mapView), in: mapView)]

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

        func handleMagnification(magnificationGestureRecognizer: NSMagnificationGestureRecognizer) {
            guard let touchHandler, let mapView else { return }

            let contentScale = Float(MCDisplayMetrics.nativeScale(for: mapView as? MTKView))
            let scale = pow(1.0 + magnificationGestureRecognizer.magnification, 2.414)
            let touch = normalizedMapPoint(magnificationGestureRecognizer.location(in: mapView), in: mapView)
            let halfWidth = 100.0

            let t0 = CGPoint(x: touch.x - halfWidth * scale, y: touch.y)
            let t1 = CGPoint(x: touch.x + halfWidth * scale, y: touch.y)
            let touches = [t0, t1]

            switch magnificationGestureRecognizer.state {
                case .began:
                    touchHandler.onTouchEvent(touches.asMCTouchEvent(scale: contentScale, action: .DOWN))
                case .changed:
                    touchHandler.onTouchEvent(touches.asMCTouchEvent(scale: contentScale, action: .MOVE))
                case .ended:
                    touchHandler.onTouchEvent(touches.asMCTouchEvent(scale: contentScale, action: .UP))
                case .failed, .cancelled:
                    touchHandler.onTouchEvent(touches.asMCTouchEvent(scale: contentScale, action: .CANCEL))
                default:
                    break
            }
        }

        func handleScroll(point: CGPoint, deltaY: CGFloat) {
            guard let touchHandler, let mapView else { return }
            let contentScale = Float(MCDisplayMetrics.nativeScale(for: mapView as? MTKView))
            let normalizedPoint = normalizedMapPoint(point, in: mapView)
            let pointers = [MCVec2F(x: Float(normalizedPoint.x) * contentScale, y: Float(normalizedPoint.y) * contentScale)]
            let event = MCTouchEvent(
                pointers: pointers,
                scrollDelta: Float(deltaY),
                touchAction: .SCROLL
            )
            touchHandler.onTouchEvent(event)
        }

        func handleClick(point: CGPoint) {
            guard let touchHandler, let mapView else { return }
            let contentScale = Float(MCDisplayMetrics.nativeScale(for: mapView as? MTKView))
            let normalizedPoint = normalizedMapPoint(point, in: mapView)
            let touches = [normalizedPoint]
            touchHandler.onTouchEvent(touches.asMCTouchEvent(scale: contentScale, action: .DOWN))
            touchHandler.onTouchEvent(touches.asMCTouchEvent(scale: contentScale, action: .UP))
        }
    #endif
}

#if canImport(UIKit)
    @MainActor
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
            MCTouchEvent(pointers: asMCTouchLocation(in: view, scale: scale), scrollDelta: 0, touchAction: action)
        }
    }
#endif

private extension [CGPoint] {
    func asMCTouchLocation(scale: Float) -> [MCVec2F] {
        map {
            let x = Float($0.x) * scale
            let y = Float($0.y) * scale
            return MCVec2F(x: x, y: y)
        }
    }

    func asMCTouchEvent(scale: Float, action: MCTouchAction) -> MCTouchEvent {
        MCTouchEvent(pointers: asMCTouchLocation(scale: scale), scrollDelta: 0, touchAction: action)
    }
}
