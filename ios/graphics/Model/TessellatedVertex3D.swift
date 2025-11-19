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

struct TessellatedVertex4F: Equatable {
    nonisolated(unsafe) static let descriptor: MTLVertexDescriptor = {
        let vertexDescriptor = MTLVertexDescriptor()
        let bufferIndex = 0
        var offset = 0
        
        // Relative Position
        vertexDescriptor.attributes[0].bufferIndex = bufferIndex
        vertexDescriptor.attributes[0].format = .float4
        vertexDescriptor.attributes[0].offset = offset
        offset += MemoryLayout<SIMD4<Float>>.stride
        
        // Absolute Position
        vertexDescriptor.attributes[1].bufferIndex = bufferIndex
        vertexDescriptor.attributes[1].format = .float4
        vertexDescriptor.attributes[1].offset = offset
        offset += MemoryLayout<SIMD4<Float>>.stride
        
        vertexDescriptor.layouts[0].stride = offset
        vertexDescriptor.layouts[0].stepRate = 1
        vertexDescriptor.layouts[0].stepFunction = .perPatchControlPoint
        return vertexDescriptor
    }()
}
