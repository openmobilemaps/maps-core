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
import MetalKit

/// A 3D point in the X-Y coordinate system carrying a texture coordinate
public struct Vertex3DTexture: Equatable {
    /// The 3D position of the vertex in the plane
    var position: SIMD3<Float>
    /// The texture coordinates mapped to the vertex in U-V coordinate space
    var textureCoordinate: SIMD2<Float>
    /// Returns the descriptor to use when passed to a metal shader
    nonisolated(unsafe) public static let descriptor: MTLVertexDescriptor = {
        let vertexDescriptor = MTLVertexDescriptor()
        var offset = 0
        let bufferIndex = 0

        // Position
        vertexDescriptor.attributes[0].bufferIndex = bufferIndex
        vertexDescriptor.attributes[0].format = .float3
        vertexDescriptor.attributes[0].offset = offset
        offset += MemoryLayout<SIMD3<Float>>.stride

        // UV
        vertexDescriptor.attributes[1].bufferIndex = bufferIndex
        vertexDescriptor.attributes[1].format = .float2
        vertexDescriptor.attributes[1].offset = offset
        offset += MemoryLayout<SIMD2<Float>>.stride

        vertexDescriptor.layouts[0].stride = MemoryLayout<Vertex3DTexture>.stride
        return vertexDescriptor
    }()

    /// Initializes a Vertex
    /// - Parameters:
    ///   - x: The X-coordinate position in the plane
    ///   - y: The Y-coordinate position in the plane
    ///   - z: The Z-coordinate position in the plane
    ///   - textureU: The texture U-coordinate mapping
    ///   - textureV: The texture V-coordinate mapping
    public init(x: Float, y: Float, z: Float, textureU: Float, textureV: Float) {
        position = SIMD3([x, y, z])
        textureCoordinate = SIMD2([textureU, textureV])
    }

    public init(position: MCVec3D, textureU: Float, textureV: Float) {
        self.init(x: position.xF, y: position.yF, z: position.zF, textureU: textureU, textureV: textureV)
    }
}

extension Vertex3DTexture {
    /// The X-coordinate position in the plane
    var x: Float { position.x }
    /// The Y-coordinate position in the plane
    var y: Float { position.y }
    /// The Z-coordinate position in the plane
    var z: Float { position.z }
    /// The texture U-coordinate mapping
    var textureU: Float { textureCoordinate.x }
    /// The texture V-coordinate mapping
    var textureV: Float { textureCoordinate.x }
}

extension Vertex3DTexture: CustomDebugStringConvertible {
    public var debugDescription: String {
        "<XYZ: (\(x), \(y), \(z)), UV: (\(textureU), \(textureV)))>"
    }
}
