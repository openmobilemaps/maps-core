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

public struct LineVertex: Equatable {
    var position: SIMD2<Float>

    /// Width Normal
    var widthNormal: SIMD2<Float>

    /// Lenght Normal
    var lenghtNormal: SIMD2<Float>

    public enum SegmantType: Int32 {
        case inner = 0
        case start = 1
        case end = 2
        case singleSegment = 3
    }

    /// Line point A
    var lineA: SIMD2<Float>

    /// Line point V
    var lineB: SIMD2<Float>

    var lenghtPrefix: Float

    var lineStyleInfo: Float

    /*
                                                  ^
                   position                       | widthNormal
                  +-------------------------------+-------------------------------+
                  |                                                               |
            <---  |  + lineA                                             lineB +  | -->
                  |                                                               |
     lenghtNormal +-------------------------------+-------------------------------+ lenghtNormal
                                                  |
                                                  v  widthNormal
      */

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

        // Width Normal
        vertexDescriptor.attributes[1].bufferIndex = bufferIndex
        vertexDescriptor.attributes[1].format = .float2
        vertexDescriptor.attributes[1].offset = offset
        offset += MemoryLayout<SIMD2<Float>>.stride

        // Length Normal
        vertexDescriptor.attributes[2].bufferIndex = bufferIndex
        vertexDescriptor.attributes[2].format = .float2
        vertexDescriptor.attributes[2].offset = offset
        offset += MemoryLayout<SIMD2<Float>>.stride

        // lineA
        vertexDescriptor.attributes[3].bufferIndex = bufferIndex
        vertexDescriptor.attributes[3].format = .float2
        vertexDescriptor.attributes[3].offset = offset
        offset += MemoryLayout<SIMD2<Float>>.stride

        // lineB
        vertexDescriptor.attributes[4].bufferIndex = bufferIndex
        vertexDescriptor.attributes[4].format = .float2
        vertexDescriptor.attributes[4].offset = offset
        offset += MemoryLayout<SIMD2<Float>>.stride

        // Length Prefix
        vertexDescriptor.attributes[5].bufferIndex = bufferIndex
        vertexDescriptor.attributes[5].format = .float
        vertexDescriptor.attributes[5].offset = offset
        offset += MemoryLayout<Float>.stride

        // Line Style Info
        vertexDescriptor.attributes[6].bufferIndex = bufferIndex
        vertexDescriptor.attributes[6].format = .float
        vertexDescriptor.attributes[6].offset = offset
        offset += MemoryLayout<Float>.stride

        vertexDescriptor.layouts[0].stride = MemoryLayout<LineVertex>.stride
        return vertexDescriptor
    }()

    init(x: Float,
         y: Float,
         lineA: MCVec2D,
         lineB: MCVec2D,
         widthNormal: (x: Float, y: Float),
         lenghtNormal: (x: Float, y: Float),
         stylingIndex: Int32 = 0,
         segmentType: SegmantType = .inner,
         lengthPrefix: Float = 0) {
        position = SIMD2(x, y)
        self.lineA = SIMD2(lineA.xF, lineA.yF)
        self.lineB = SIMD2(lineB.xF, lineB.yF)
        self.widthNormal = SIMD2(widthNormal.x, widthNormal.y)
        self.lenghtNormal = SIMD2(lenghtNormal.x, lenghtNormal.y)
        lenghtPrefix = lengthPrefix

        let st : Int32 = segmentType.rawValue << 8
        lineStyleInfo = Float(Int32(stylingIndex) + st)
    }
}
