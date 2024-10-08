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

struct PolygonVertex: Equatable {
    var position: SIMD2<Float>
    var stylingIndex: Float = 0.0

    /// Returns the descriptor to use when passed to a metal shader
    nonisolated(unsafe) static let descriptor: MTLVertexDescriptor = {
        let vertexDescriptor = MTLVertexDescriptor()
        var offset = 0
        let bufferIndex = 0

        // Position
        vertexDescriptor.attributes[0].bufferIndex = bufferIndex
        vertexDescriptor.attributes[0].format = .float2
        vertexDescriptor.attributes[0].offset = offset
        offset += MemoryLayout<SIMD2<Float>>.stride

        // Styling Index
        vertexDescriptor.attributes[1].bufferIndex = bufferIndex
        vertexDescriptor.attributes[1].format = .float
        vertexDescriptor.attributes[1].offset = offset
        offset += MemoryLayout<Float>.stride

        vertexDescriptor.layouts[0].stride = offset
        return vertexDescriptor
    }()
}

struct PolygonVertex4: Equatable {
    /// Returns the descriptor to use when passed to a metal shader
    nonisolated(unsafe) static let descriptor: MTLVertexDescriptor = {
        let vertexDescriptor = MTLVertexDescriptor()
        var offset = 0
        let bufferIndex = 0

        // Position + StylingIndex
        vertexDescriptor.attributes[0].bufferIndex = bufferIndex
        vertexDescriptor.attributes[0].format = .float4
        vertexDescriptor.attributes[0].offset = offset
        offset += MemoryLayout<SIMD4<Float>>.stride

        vertexDescriptor.layouts[0].stride = offset
        return vertexDescriptor
    }()
}
