//
//  ElevationInterpolationShader.swift
//  MapCore
//
//  Created by Nicolas Märki on 18.11.2025.
//

open class ElevationInterpolationShader: BaseShader, @unchecked Sendable {
    public init() {
        super.init(shader: .elevationInterpolation)
    }
}

extension ElevationInterpolationShader: MCElevationInterpolationShaderInterface {
    public func asShaderProgram() -> (any MCShaderProgramInterface)? {
        self
    }
}
