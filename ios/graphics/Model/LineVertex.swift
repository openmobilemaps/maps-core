/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import MetalKit
import MapCoreSharedModule

struct LineVertex: Equatable {

    var position: SIMD2<Float>

    /// Line point A
    var lineA: SIMD2<Float>
    /// Line point V
    var lineB: SIMD2<Float>

    /// Returns the descriptor to use when passed to a metal shader
    static let descriptor: MTLVertexDescriptor = {
        let vertexDescriptor = MTLVertexDescriptor()
        var offset = 0
        let bufferIndex = 0

        // Position
        vertexDescriptor.attributes[0].bufferIndex = bufferIndex
        vertexDescriptor.attributes[0].format = .float2
        vertexDescriptor.attributes[0].offset = offset
        offset += MemoryLayout<SIMD2<Float>>.stride

        // lineA
        vertexDescriptor.attributes[1].bufferIndex = bufferIndex
        vertexDescriptor.attributes[1].format = .float2
        vertexDescriptor.attributes[1].offset = offset
        offset += MemoryLayout<SIMD2<Float>>.stride

        // lineB
        vertexDescriptor.attributes[2].bufferIndex = bufferIndex
        vertexDescriptor.attributes[2].format = .float2
        vertexDescriptor.attributes[2].offset = offset
        offset += MemoryLayout<SIMD2<Float>>.stride

        vertexDescriptor.layouts[0].stride = MemoryLayout<LineVertex>.stride
        return vertexDescriptor
    }()
    
    init(x: Float, y: Float, lineA: MCVec2D, lineB: MCVec2D) {
        position = SIMD2([x, y])
        self.lineA = SIMD2([lineA.xF, lineA.yF])
        self.lineB = SIMD2([lineB.xF, lineB.yF])
    }
}
