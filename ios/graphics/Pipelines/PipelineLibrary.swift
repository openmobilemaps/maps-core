/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import Metal

public enum PipelineDescriptorFactory {
    public static func pipelineDescriptor(vertexDescriptor: MTLVertexDescriptor,
                                          label: String,
                                          vertexShader: String,
                                          fragmentShader: String,
                                          library: MTLLibrary = MetalContext.current.library) -> MTLRenderPipelineDescriptor {
        let pipelineDescriptor = MTLRenderPipelineDescriptor()
        pipelineDescriptor.colorAttachments[0].pixelFormat = MetalContext.current.colorPixelFormat
        pipelineDescriptor.vertexDescriptor = vertexDescriptor

        let renderbufferAttachment = pipelineDescriptor.colorAttachments[0]
        renderbufferAttachment?.pixelFormat = MetalContext.current.colorPixelFormat
        renderbufferAttachment?.isBlendingEnabled = true
        renderbufferAttachment?.rgbBlendOperation = .add
        renderbufferAttachment?.alphaBlendOperation = .add
        renderbufferAttachment?.sourceRGBBlendFactor = .one
        renderbufferAttachment?.sourceAlphaBlendFactor = .one
        renderbufferAttachment?.destinationRGBBlendFactor = .oneMinusSourceAlpha
        renderbufferAttachment?.destinationAlphaBlendFactor = .oneMinusSourceAlpha

        pipelineDescriptor.stencilAttachmentPixelFormat = .stencil8
        pipelineDescriptor.label = label

        guard let vertexFunction = library.makeFunction(name: vertexShader),
              let fragmentFunction = library.makeFunction(name: fragmentShader)
        else {
            fatalError("Cannot locate the shaders for \(label)")
        }

        pipelineDescriptor.vertexFunction = vertexFunction
        pipelineDescriptor.fragmentFunction = fragmentFunction

        return pipelineDescriptor
    }
}

extension PipelineDescriptorFactory {
    static func pipelineDescriptor(pipeline: Pipeline) -> MTLRenderPipelineDescriptor {
        pipelineDescriptor(vertexDescriptor: pipeline.vertexDescriptor,
                           label: pipeline.label,
                           vertexShader: pipeline.vertexShader,
                           fragmentShader: pipeline.fragmentShader)
    }
}

public enum Pipeline: String, CaseIterable {
    case alphaShader
    case alphaInstancedShader
    case lineGroupShader
    case lineGroupInstancedShader
    case polygonGroupShader
    case colorShader
    case roundColorShader
    case clearStencilShader
    case textShader
    case rasterShader
    case stretchShader

    var label: String {
        switch self {
            case .alphaShader: return "Alpha shader with texture"
            case .alphaInstancedShader: return "Alpha instanced shader with texture"
            case .lineGroupShader: return "Line Group shader"
            case .lineGroupInstancedShader: return "Line Group Instanced shader"
            case .polygonGroupShader: return "Polygon Group shader"
            case .colorShader: return "Color shader"
            case .roundColorShader: return "Round color shader"
            case .clearStencilShader: return "Clear stencil shader"
            case .textShader: return "Text shader"
            case .rasterShader: return "Raster shader"
            case .stretchShader: return "Stretch shader"
        }
    }

    var vertexShader: String {
        switch self {
            case .alphaShader: return "baseVertexShader"
            case .alphaInstancedShader: return "alphaInstancedVertexShader"
            case .lineGroupShader: return "lineGroupVertexShader"
            case .lineGroupInstancedShader: return "lineGroupInstancedVertexShader"
            case .polygonGroupShader: return "polygonGroupVertexShader"
            case .colorShader: return "colorVertexShader"
            case .roundColorShader: return "colorVertexShader"
            case .clearStencilShader: return "stencilClearVertexShader"
            case .textShader: return "textVertexShader"
            case .rasterShader: return "rasterVertexShader"
            case .stretchShader: return "stretchVertexShader"
        }
    }

    var fragmentShader: String {
        switch self {
            case .alphaShader: return "baseFragmentShader"
            case .alphaInstancedShader: return "alphaInstancedFragmentShader"
            case .lineGroupShader: return "lineGroupFragmentShader"
            case .lineGroupInstancedShader: return "lineGroupInstancedFragmentShader"
            case .polygonGroupShader: return "polygonGroupFragmentShader"
            case .colorShader: return "colorFragmentShader"
            case .roundColorShader: return "roundColorFragmentShader"
            case .clearStencilShader: return "stencilClearFragmentShader"
            case .textShader: return "textFragmentShader"
            case .rasterShader: return "rasterFragmentShader"
            case .stretchShader: return "stretchFragmentShader"
        }
    }

    var vertexDescriptor: MTLVertexDescriptor {
        switch self {
            case .lineGroupShader: return LineVertex.descriptor
            case .polygonGroupShader: return PolygonVertex.descriptor
            default: return Vertex.descriptor
        }
    }
}

public class PipelineLibrary: StaticMetalLibrary<String, MTLRenderPipelineState> {
    init(device: MTLDevice) throws {
        try super.init(Pipeline.allCases.map(\.rawValue)) { key -> MTLRenderPipelineState in
            guard let pipeline = Pipeline(rawValue: key) else {
                throw LibraryError.invalidKey
            }
            let pipelineDescriptor = PipelineDescriptorFactory.pipelineDescriptor(pipeline: pipeline)
            return try device.makeRenderPipelineState(descriptor: pipelineDescriptor)
        }
    }
}
