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

/// A 3D point in the X-Y coordinate system carrying a texture coordinate
public struct TessellatedVertex3DTexture: Equatable {
    /// The 3D position of the vertex in the plane
    var relativePosition: SIMD3<Float>
    /// The 3D position of the vertex in the world
    var absolutePosition: SIMD2<Float>
    /// The texture coordinates mapped to the vertex in U-V coordinate space
    var textureCoordinate: SIMD2<Float>
    /// Returns the descriptor to use when passed to a metal shader
    nonisolated(unsafe) public static let descriptor: MTLVertexDescriptor = {
        let vertexDescriptor = MTLVertexDescriptor()
        var offset = 0
        let bufferIndex = 0

        // Relative Position
        vertexDescriptor.attributes[0].bufferIndex = bufferIndex
        vertexDescriptor.attributes[0].format = .float3
        vertexDescriptor.attributes[0].offset = offset
        offset += MemoryLayout<SIMD3<Float>>.stride
        
        // Absolute Position
        vertexDescriptor.attributes[1].bufferIndex = bufferIndex
        vertexDescriptor.attributes[1].format = .float2
        vertexDescriptor.attributes[1].offset = offset
        offset += MemoryLayout<SIMD2<Float>>.stride

        // UV
        vertexDescriptor.attributes[2].bufferIndex = bufferIndex
        vertexDescriptor.attributes[2].format = .float2
        vertexDescriptor.attributes[2].offset = offset
        offset += MemoryLayout<SIMD2<Float>>.stride
        
        vertexDescriptor.layouts[0].stride = MemoryLayout<TessellatedVertex3DTexture>.stride
        vertexDescriptor.layouts[0].stepRate = 1
        vertexDescriptor.layouts[0].stepFunction = .perPatchControlPoint
        return vertexDescriptor
    }()

    /// Initializes a Vertex
    /// - Parameters:
    ///   - relativeX: The X-coordinate position in the plane
    ///   - relativeY: The Y-coordinate position in the plane
    ///   - relativeZ: The Z-coordinate position in the plane
    ///   - absoluteX: The X-coordinate position in the world
    ///   - absoluteY: The Y-coordinate position in the world
    ///   - absoluteZ: The Z-coordinate position in the world
    ///   - textureU: The texture U-coordinate mapping
    ///   - textureV: The texture V-coordinate mapping
    public init(
        relativeX: Float, relativeY: Float, relativeZ: Float,
        absoluteX: Float, absoluteY: Float,
        textureU: Float, textureV: Float,
    ) {
        relativePosition = SIMD3([relativeX, relativeY, relativeZ])
        absolutePosition = SIMD2([absoluteX, absoluteY])
        textureCoordinate = SIMD2([textureU, textureV])
    }

    public init(relativePosition: MCVec3D, absolutePositionX: Float, absolutePositionY: Float, textureU: Float, textureV: Float) {
        self.init(
            relativeX: relativePosition.xF, relativeY: relativePosition.yF, relativeZ: relativePosition.zF,
            absoluteX: absolutePositionX, absoluteY: absolutePositionY,
            textureU: textureU, textureV: textureV,
        )
    }
}

extension TessellatedVertex3DTexture {
    /// The X-coordinate position in the plane
    var relativeX: Float { relativePosition.x }
    /// The Y-coordinate position in the plane
    var relativeY: Float { relativePosition.y }
    /// The Z-coordinate position in the plane
    var relativeZ: Float { relativePosition.z }
    /// The X-coordinate position in the world
    var absoluteX: Float { absolutePosition.x }
    /// The Y-coordinate position in the world
    var absoluteY: Float { absolutePosition.y }
    /// The texture U-coordinate mapping
    var textureU: Float { textureCoordinate.x }
    /// The texture V-coordinate mapping
    var textureV: Float { textureCoordinate.x }
}

extension TessellatedVertex3DTexture: CustomDebugStringConvertible {
    public var debugDescription: String {
        "<RelativeXYZ: (\(relativeX), \(relativeY), \(relativeZ)), AbsoluteXY: (\(absoluteX), \(absoluteY)), UV: (\(textureU), \(textureV))>"
    }
}
