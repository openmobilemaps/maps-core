//
//  CameraConfig.swift
//  MapCore
//
//  Created by Nicolas Märki on 16.08.2025.
//

import SwiftUI

public struct CameraConfig: Sendable, Equatable {

    public static let basic: CameraConfig = CameraConfig(mcConfig: MCCamera3dConfigFactory.getBasicConfig())

    public static let restor: CameraConfig = CameraConfig(mcConfig: MCCamera3dConfigFactory.getRestorConfig())

    public static func custom(_ config: MCCamera3dConfig) -> CameraConfig {
        CameraConfig(mcConfig: config)
    }

    private init(mcConfig: MCCamera3dConfig) {
        self.mcConfig = mcConfig
    }

    let mcConfig: MCCamera3dConfig

    public static func ==(lhs: CameraConfig, rhs: CameraConfig) -> Bool {
        lhs.mcConfig.key == rhs.mcConfig.key
    }
}

extension CameraConfig: EnvironmentKey {
    public static let defaultValue: CameraConfig = .basic
}

extension EnvironmentValues {
    var mapCameraConfig: CameraConfig {
        get { self[CameraConfig.self] }
        set { self[CameraConfig.self] = newValue }
    }
}

// 2) View-Modifier / Convenience-API
extension View {
    /// Setzt eine globale Hintergrundfarbe für Map-Views im View-Hierarchie-Teil.
    public func mapCameraConfig(_ cameraConfig: CameraConfig) -> some View {
        environment(\.mapCameraConfig, cameraConfig)
    }
}
