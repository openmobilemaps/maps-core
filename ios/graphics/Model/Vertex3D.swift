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

struct Vertex3F: Equatable {
    nonisolated(unsafe) static let descriptor: MTLVertexDescriptor = {
        let vertexDescriptor = MTLVertexDescriptor()
        
        vertexDescriptor.attributes[0].bufferIndex = 0
        vertexDescriptor.attributes[0].format = .float3
        vertexDescriptor.attributes[0].offset = 0

        vertexDescriptor.layouts[0].stride = MemoryLayout<SIMD3<Float>>.stride
        return vertexDescriptor
    }()
}

struct Vertex4F: Equatable {
    nonisolated(unsafe) static let descriptor: MTLVertexDescriptor = {
        let vertexDescriptor = MTLVertexDescriptor()

        vertexDescriptor.attributes[0].bufferIndex = 0
        vertexDescriptor.attributes[0].format = .float4
        vertexDescriptor.attributes[0].offset = 0

        vertexDescriptor.layouts[0].stride = MemoryLayout<SIMD4<Float>>.stride
        return vertexDescriptor
    }()
}
