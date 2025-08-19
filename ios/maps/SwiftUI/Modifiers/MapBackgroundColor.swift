//
//  MapBackgroundColorKey.swift
//  MapCore
//
//  Created by Nicolas Märki on 15.08.2025.
//

import SwiftUI

private struct MapBackgroundColorKey: EnvironmentKey {
    static let defaultValue: Color = .white
}

extension EnvironmentValues {
    var mapBackgroundColor: Color {
        get { self[MapBackgroundColorKey.self] }
        set { self[MapBackgroundColorKey.self] = newValue }
    }
}

// 2) View-Modifier / Convenience-API
extension View {
    /// Setzt eine globale Hintergrundfarbe für Map-Views im View-Hierarchie-Teil.
    public func mapBackgroundColor(_ color: Color) -> some View {
        environment(\.mapBackgroundColor, color)
    }
}
