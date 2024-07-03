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
    /// Returns the descriptor to use when passed to a metal shader
    nonisolated(unsafe) public static let descriptor: MTLVertexDescriptor = {
        let vertexDescriptor = MTLVertexDescriptor()
        var offset = 0
        let bufferIndex = 0

        // Position
        vertexDescriptor.attributes[0].bufferIndex = bufferIndex
        vertexDescriptor.attributes[0].format = .float2
        vertexDescriptor.attributes[0].offset = offset
        offset += 2 * MemoryLayout<Float>.stride

        // Width Normal
        vertexDescriptor.attributes[1].bufferIndex = bufferIndex
        vertexDescriptor.attributes[1].format = .float2
        vertexDescriptor.attributes[1].offset = offset
        offset += 2 * MemoryLayout<Float>.stride

        // lineA
        vertexDescriptor.attributes[2].bufferIndex = bufferIndex
        vertexDescriptor.attributes[2].format = .float2
        vertexDescriptor.attributes[2].offset = offset
        offset += 2 * MemoryLayout<Float>.stride

        // lineB
        vertexDescriptor.attributes[3].bufferIndex = bufferIndex
        vertexDescriptor.attributes[3].format = .float2
        vertexDescriptor.attributes[3].offset = offset
        offset += 2 * MemoryLayout<Float>.stride

        // Vertex Index
        vertexDescriptor.attributes[4].bufferIndex = bufferIndex
        vertexDescriptor.attributes[4].format = .float
        vertexDescriptor.attributes[4].offset = offset
        offset += MemoryLayout<Float>.stride

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

        vertexDescriptor.layouts[0].stride = offset
        return vertexDescriptor
    }()
}
