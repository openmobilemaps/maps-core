//
//  MultiBuffer.swift
//  MapCore
//
//  Created by Nicolas Märki on 04.11.2024.
//

import Metal
import simd

public struct MultiBuffer<DataType> {
    // Warning: Seems like suited to be generic
    // But makeBuffer &reference does not like generic data
    var buffers: [MTLBuffer]

    fileprivate init(buffers: [MTLBuffer]) {
        self.buffers = buffers
    }

    public mutating func getNextBuffer(_ context: RenderingContext)
        -> MTLBuffer?
    {
        return buffers[context.currentBufferIndex]
    }
}

extension MultiBuffer<simd_float1> {
    public init(device: MTLDevice) {
        var initialMutable = simd_float1(1.0)
        let buffers: [MTLBuffer] = (0..<RenderingContext.bufferCount).compactMap
        { _ in
            device
                .makeBuffer(
                    bytes: &initialMutable,
                    length: MemoryLayout<simd_float1>.stride
                )
        }
        self.init(buffers: buffers)
    }
}

extension MultiBuffer<simd_float4> {
    public init(device: MTLDevice) {
        var initialMutable = simd_float4(0.0, 0.0, 0.0, 0.0)
        let buffers: [MTLBuffer] = (0..<RenderingContext.bufferCount).compactMap
        { _ in
            device
                .makeBuffer(
                    bytes: &initialMutable,
                    length: MemoryLayout<simd_float4>.stride
                )
        }
        self.init(buffers: buffers)
    }
}

extension MultiBuffer<simd_float4x4> {
    public init(device: MTLDevice) {
        var initialMutable = simd_float4x4(1.0)
        let buffers: [MTLBuffer] = (0..<RenderingContext.bufferCount).compactMap
        { _ in
            device
                .makeBuffer(
                    bytes: &initialMutable,
                    length: MemoryLayout<simd_float4x4>.stride
                )
        }
        self.init(buffers: buffers)
    }
}
