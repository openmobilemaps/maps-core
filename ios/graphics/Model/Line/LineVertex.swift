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

public struct LineVertex: Equatable {
    /// Returns the descriptor to use when passed to a metal shader
    nonisolated(unsafe) public static let descriptorUnitSphere: MTLVertexDescriptor = {
        let vertexDescriptor = MTLVertexDescriptor()
        var offset = 0
        let bufferIndex = 0

        // Position
        vertexDescriptor.attributes[0].bufferIndex = bufferIndex
        vertexDescriptor.attributes[0].format = .float3
        vertexDescriptor.attributes[0].offset = offset
        offset += 3 * MemoryLayout<Float>.stride

        // Extrude
        vertexDescriptor.attributes[1].bufferIndex = bufferIndex
        vertexDescriptor.attributes[1].format = .float3
        vertexDescriptor.attributes[1].offset = offset
        offset += 3 * MemoryLayout<Float>.stride

        // Side
        vertexDescriptor.attributes[2].bufferIndex = bufferIndex
        vertexDescriptor.attributes[2].format = .float
        vertexDescriptor.attributes[2].offset = offset
        offset += MemoryLayout<Float>.stride

        // Length Prefix
        vertexDescriptor.attributes[3].bufferIndex = bufferIndex
        vertexDescriptor.attributes[3].format = .float
        vertexDescriptor.attributes[3].offset = offset
        offset += MemoryLayout<Float>.stride

        // Line Style Info
        vertexDescriptor.attributes[4].bufferIndex = bufferIndex
        vertexDescriptor.attributes[4].format = .float
        vertexDescriptor.attributes[4].offset = offset
        offset += MemoryLayout<Float>.stride

        vertexDescriptor.layouts[0].stride = offset
        return vertexDescriptor
    }()

    nonisolated(unsafe) public static let descriptor: MTLVertexDescriptor = {
        let vertexDescriptor = MTLVertexDescriptor()
        var offset = 0
        let bufferIndex = 0

        // Postion
        vertexDescriptor.attributes[0].bufferIndex = bufferIndex
        vertexDescriptor.attributes[0].format = .float2
        vertexDescriptor.attributes[0].offset = offset
        offset += 2 * MemoryLayout<Float>.stride

        // Extrude
        vertexDescriptor.attributes[1].bufferIndex = bufferIndex
        vertexDescriptor.attributes[1].format = .float2
        vertexDescriptor.attributes[1].offset = offset
        offset += 2 * MemoryLayout<Float>.stride

        // Side
        vertexDescriptor.attributes[2].bufferIndex = bufferIndex
        vertexDescriptor.attributes[2].format = .float
        vertexDescriptor.attributes[2].offset = offset
        offset += MemoryLayout<Float>.stride

        // Length Prefix
        vertexDescriptor.attributes[3].bufferIndex = bufferIndex
        vertexDescriptor.attributes[3].format = .float
        vertexDescriptor.attributes[3].offset = offset
        offset += MemoryLayout<Float>.stride

        // Line Style Info
        vertexDescriptor.attributes[4].bufferIndex = bufferIndex
        vertexDescriptor.attributes[4].format = .float
        vertexDescriptor.attributes[4].offset = offset
        offset += MemoryLayout<Float>.stride

        vertexDescriptor.layouts[0].stride = offset
        return vertexDescriptor
    }()
}
