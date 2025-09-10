//
//  MapPaddingKey.swift
//  MapCore
//
//  Created by Nicolas Märki on 17.08.2025.
//


import SwiftUI

private struct MapPaddingKey: EnvironmentKey {
    static let defaultValue: EdgeInsets = .init()
}
extension EnvironmentValues {
    var mapPadding: EdgeInsets {
        get { self[MapPaddingKey.self] }
        set { self[MapPaddingKey.self] = newValue }
    }
}
extension View {
    public func mapPadding(_ insets: EdgeInsets) -> some View {
        environment(\.mapPadding, insets)
    }
}
