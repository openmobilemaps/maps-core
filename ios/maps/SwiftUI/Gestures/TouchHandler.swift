//
//  TouchHandler.swift
//  MapCore
//
//  Created by Nicolas Märki on 20.08.2025.
//




class TouchHandler: MCTouchInterface {

    var gestures: [any MapGesture] = []
    var camera: MCMapCameraInterface?

    public init() {}

    open func onTouchDown(_ posScreen: MCVec2F) -> Bool {
        handleMapTouchGestures(posScreen, type: .down)
    }

    open func onClickUnconfirmed(_ posScreen: MCVec2F) -> Bool {
        handleMapTouchGestures(posScreen, type: .unconfirmed)
    }

    open func onClickConfirmed(_ posScreen: MCVec2F) -> Bool {
        handleMapTouchGestures(posScreen, type: .confirmed)
    }

    open func onDoubleClick(_ posScreen: MCVec2F) -> Bool {
        handleMapTouchGestures(posScreen, type: .double)
    }

    open func onMove(_ deltaScreen: MCVec2F, confirmed: Bool, doubleClick: Bool) -> Bool {
        false
    }

    open func onMoveComplete() -> Bool {
        false
    }

    public func onOneFingerDoubleClickMoveComplete() -> Bool {
        false
    }

    open func onTwoFingerClick(_ posScreen1: MCVec2F, posScreen2: MCVec2F) -> Bool {
        false
    }

    open func onTwoFingerMove(_ posScreenOld: [MCVec2F], posScreenNew: [MCVec2F]) -> Bool {
        false
    }

    open func onTwoFingerMoveComplete() -> Bool {
        false
    }

    open func clearTouch() {}

    open func onLongPress(_ posScreen: MCVec2F) -> Bool {
        handleMapTouchGestures(posScreen, type: .long)
    }

    private func handleMapTouchGestures(_ posScreen: MCVec2F, type: TouchType) -> Bool {
        var coord = (self.camera?.coord(fromScreenPosition: posScreen)).map { Coordinate($0) }
        let gestures = self.gestures
        if Thread.isMainThread {
            return MainActor.assumeIsolated {
                if coord?.systemIdentifier == -1 {
                    coord = nil
                }
                for gesture in gestures {
                    if let gesture = gesture as? MapTouchGesture,
                        gesture.type == .down,
                        gesture.action(coord)
                    {
                        return true
                    }
                }
                return false
            }
        } else {
            return DispatchQueue.main.sync {
                if coord?.systemIdentifier == -1 {
                    coord = nil
                }
                for gesture in gestures {
                    if let gesture = gesture as? MapTouchGesture,
                        gesture.type == .down,
                        gesture.action(coord)
                    {
                        return true
                    }
                }
                return false
            }
        }
    }
}