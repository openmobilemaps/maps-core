//
//  Projection.swift
//  MapCore
//
//  Created by Nicolas Märki on 16.08.2025.
//

import SwiftUI

public enum Projection: Sendable {
    case spherical
    case epsg3857
    case epsg4326
    case epsg21781
}

extension Projection {
    var coordinateSystem: MCMapCoordinateSystem {
        switch self {
            case .spherical:
                MCCoordinateSystemFactory.getUnitSphereSystem()
            case .epsg3857:
                MCCoordinateSystemFactory.getEpsg3857System()
            case .epsg4326:
                MCCoordinateSystemFactory.getEpsg4326System()
            case .epsg21781:
                MCCoordinateSystemFactory.getEpsg21781System()
        }
    }

    var is3D: Bool {
        self == .spherical
    }
}

extension Projection: EnvironmentKey {
    public static let defaultValue: Projection = .spherical
}

extension EnvironmentValues {
    var mapProjection: Projection {
        get { self[Projection.self] }
        set { self[Projection.self] = newValue }
    }
}

// 2) View-Modifier / Convenience-API
extension View {
    /// Setzt eine globale Hintergrundfarbe für Map-Views im View-Hierarchie-Teil.
    public func mapProjection(_ projection: Projection) -> some View {
        environment(\.mapProjection, projection)
    }
}

