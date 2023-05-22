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

/// A 3D point in the X-Y-Z coordinate system carrying a texture coordinate
struct Vertex3D: Equatable {
    /// The 3D position of the vertex in the plane
    var position: SIMD4<Float>
    /// The texture coordinates mapped to the vertex in U-V coordinate space
    var textureCoordinate: SIMD2<Float>
    /// Normal
//    var normal: SIMD3<Float>

    /// Returns the descriptor to use when passed to a metal shader
    static let descriptor: MTLVertexDescriptor = {
        let vertexDescriptor = MTLVertexDescriptor()
        var offset = 0
        let bufferIndex = 0

        // Position
        vertexDescriptor.attributes[0].bufferIndex = bufferIndex
        vertexDescriptor.attributes[0].format = .float3
        vertexDescriptor.attributes[0].offset = offset
        assert(MemoryLayout<SIMD4<Float>>.stride == 16)
        offset += 16

        // UV
        vertexDescriptor.attributes[1].bufferIndex = bufferIndex
        vertexDescriptor.attributes[1].format = .float2
        vertexDescriptor.attributes[1].offset = offset
        assert(MemoryLayout<SIMD2<Float>>.stride == 8)
        offset += 8

        // Normal
//        vertexDescriptor.attributes[2].bufferIndex = bufferIndex
//        vertexDescriptor.attributes[2].format = .float3
//        vertexDescriptor.attributes[2].offset = offset
//        offset += MemoryLayout<SIMD2<Float>>.stride

        // TODO: Size or stride?
        assert(MemoryLayout<Vertex3D>.stride == 32)
        vertexDescriptor.layouts[0].stride = 32
        return vertexDescriptor
    }()

    public static let tesselatedDescriptor: MTLVertexDescriptor = {
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

//        // Normal
//        vertexDescriptor.attributes[2].bufferIndex = bufferIndex
//        vertexDescriptor.attributes[2].format = .float3
//        vertexDescriptor.attributes[2].offset = offset
//        offset += MemoryLayout<SIMD2<Float>>.stride
//
//        // lineStart
//        vertexDescriptor.attributes[3].bufferIndex = bufferIndex
//        vertexDescriptor.attributes[3].format = .float2
//        vertexDescriptor.attributes[3].offset = offset
//        offset += MemoryLayout<SIMD2<Float>>.stride
//
//        // lineEnd
//        vertexDescriptor.attributes[4].bufferIndex = bufferIndex
//        vertexDescriptor.attributes[4].format = .float2
//        vertexDescriptor.attributes[4].offset = offset
//        offset += MemoryLayout<SIMD2<Float>>.stride

        vertexDescriptor.layouts[0].stride = MemoryLayout<Vertex3D>.stride
        vertexDescriptor.layouts[0].stepFunction = .perPatchControlPoint
        vertexDescriptor.layouts[0].stepRate = 1

        return vertexDescriptor
    }()

    init(x: Float, y: Float, z: Float) {
        position = SIMD4([x, y, z, 1.0])
//        normal = SIMD3([0.0, 0.0, 0.0])
        textureCoordinate = SIMD2([0.0, 0.0])
    }

    /// Initializes a Vertex
    /// - Parameters:
    ///   - x: The X-coordinate position in the plane
    ///   - y: The Y-coordinate position in the plane
    ///   - textureU: The texture U-coordinate mapping
    ///   - textureV: The texture V-coordinate mapping
    init(x: Float, y: Float, z: Float, textureU: Float, textureV: Float) {
        position = SIMD4([x, y, z, 1.0])
//        normal = SIMD3([0.0, 0.0, 0.0])
        textureCoordinate = SIMD2([textureU, textureV])
    }

    /// Initializes a Vertex
    /// - Parameters:
    ///   - x: The X-coordinate position in the plane
    ///   - y: The Y-coordinate position in the plane
    ///   - textureU: The texture U-coordinate mapping
    ///   - textureV: The texture V-coordinate mapping
    init(x: Float, y: Float, z: Float, normalX: Float, normalY: Float, normalZ: Float, textureU: Float, textureV: Float) {
        position = SIMD4([x, y, z, 1.0])
//        normal = SIMD3([normalX, normalY, normalZ])
        textureCoordinate = SIMD2([textureU, textureV])
    }

    init(x: Float, y: Float, z: Float, normalX: Float, normalY: Float, normalZ: Float) {
        position = SIMD4([x, y, z, 1.0])
//        normal = SIMD3([normalX, normalY, normalZ])
        textureCoordinate = SIMD2([0.0, 0.0])
    }

    init(position: MCVec3D, textureU: Float, textureV: Float) {
        self.init(x: position.xF, y: position.yF, z: position.zF, textureU: textureU, textureV: textureV)
    }
}

extension Vertex3D {
    /// The X-coordinate position in the plane
    var x: Float { position.x }
    /// The Y-coordinate position in the plane
    var y: Float { position.y }
    /// The Y-coordinate position in depth
    var z: Float { position.z }
    /// The texture U-coordinate mapping
    var textureU: Float { textureCoordinate.x }
    /// The texture V-coordinate mapping
    var textureV: Float { textureCoordinate.x }

//    var normalX: Float { normal.x }
//    var normalY: Float { normal.y }
}

extension Vertex3D: CustomDebugStringConvertible {
    var debugDescription: String {
        "<XYZ: (\(x), \(y)), \(z)), UV: (\(textureU), \(textureV))>"
    }
}
