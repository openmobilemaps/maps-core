//
//  MapZoomLimit.swift
//  MapCore
//
//  Created by Nicolas Märki on 15.08.2025.
//

import SwiftUI

private struct MapZoomLimitKey: EnvironmentKey {
    static let defaultValue: (min: Double?, max: Double?) = (nil, nil)
}

extension EnvironmentValues {
    var mapZoomLimit: (min: Double?, max: Double?) {
        get { self[MapZoomLimitKey.self] }
        set { self[MapZoomLimitKey.self] = newValue }
    }
}

extension View {
    /// Setzt globale Zoomgrenzen (min/max) für Map-Views im View-Hierarchie-Teil.
    public func mapZoomLimit(min: Double?, max: Double?) -> some View {
        environment(\.mapZoomLimit, (min, max))
    }
}
