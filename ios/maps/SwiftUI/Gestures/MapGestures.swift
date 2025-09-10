//
//  MapTapHandlersKey.swift
//  MapCore
//
//  Created by Nicolas Märki on 19.08.2025.
//

import SwiftUI

protocol MapGesture: Sendable {

}

public enum TouchType: Sendable {
    case down
    case confirmed
    case unconfirmed
    case long
    case double
}

struct MapTouchGesture: MapGesture {
    let action: @MainActor (Coordinate?) -> Bool
    let type: TouchType
}

private struct MapGesturesKey: EnvironmentKey {
    static let defaultValue: [any MapGesture] = []
}

extension EnvironmentValues {
    var mapGestures: [any MapGesture] {
        get { self[MapGesturesKey.self] }
        set { self[MapGesturesKey.self] = newValue }
    }
}

struct MapGestureModifier: ViewModifier {
    @Environment(\.mapGestures) private var existing

    let gesture: any MapGesture

    func body(content: Content) -> some View {
        content.environment(\.mapGestures, existing + [gesture])
    }
}

extension View {
    //    public func onMapTapGesture(_ type: TouchType = .confirmed, perform action: @escaping @MainActor (Coordinate?) -> Bool) -> some View {
    //        self.modifier(MapGestureModifier(gesture: MapTouchGesture(action: action, type: type)))
    //    }
    public func onMapTapGesture(_ type: TouchType = .confirmed, perform action: @escaping @MainActor (Coordinate?) -> Void) -> some View {
        self.modifier(
            MapGestureModifier(
                gesture: MapTouchGesture(
                    action: {
                        action($0)
                        return false
                    }, type: type)))
    }

    public func onMapTapGesture(_ type: TouchType = .confirmed, perform action: @escaping @MainActor (Coordinate?, inout Bool) -> Void) -> some View {
        self.modifier(
            MapGestureModifier(
                gesture: MapTouchGesture(
                    action: {
                        var stop = true
                        action($0, &stop)
                        return stop
                    }, type: type)))
    }
}
