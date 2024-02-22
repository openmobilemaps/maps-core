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
public struct IcosahedronVertex {
    var position: SIMD2<Float>

    var value: Float

    /// Returns the descriptor to use when passed to a metal shader
    public static let descriptor: MTLVertexDescriptor = {
        let vertexDescriptor = MTLVertexDescriptor()
        var offset = 0
        let bufferIndex = 0

        // Position
        vertexDescriptor.attributes[0].bufferIndex = bufferIndex
        vertexDescriptor.attributes[0].format = .float2
        vertexDescriptor.attributes[0].offset = offset
        offset += MemoryLayout<SIMD2<Float>>.stride

        // Value
        vertexDescriptor.attributes[1].bufferIndex = bufferIndex
        vertexDescriptor.attributes[1].format = .float
        vertexDescriptor.attributes[1].offset = offset
        offset += MemoryLayout<Float>.stride

        vertexDescriptor.layouts[0].stride = MemoryLayout<IcosahedronVertex>.stride
        return vertexDescriptor
    }()

    public init(position: MCVec2D, value: Float) {
        self.position = SIMD2([position.xF, position.yF])
        self.value = value
    }
}
