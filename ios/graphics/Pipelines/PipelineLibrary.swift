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
            fatalError("Cannot locate the shaders for UBTileModel")
        }

        pipelineDescriptor.vertexFunction = vertexFunction
        pipelineDescriptor.fragmentFunction = fragmentFunction

        return pipelineDescriptor
    }
}

extension PipelineDescriptorFactory {
    static func pipelineDescriptor(pipeline: Pipeline) -> MTLRenderPipelineDescriptor {
        return pipelineDescriptor(vertexDescriptor: pipeline.vertexDescriptor,
                                  label: pipeline.label,
                                  vertexShader: pipeline.vertexShader,
                                  fragmentShader: pipeline.fragmentShader)
    }
}

public enum Pipeline: String, CaseIterable {
    case alphaShader = "alphaShader"
    case lineShader = "lineShader"
    case lineGroupShader = "lineGroupShader"
    case polygonGroupShader = "polygonGroupShader"
    case pointShader = "pointShader"
    case colorShader = "colorShader"
    case roundColorShader = "roundColorShader"
    case clearStencilShader = "clearStencilShader"
    case textShader = "textShader"

    var label: String {
        switch self {
        case .alphaShader: return "Alpha shader with texture"
        case .lineShader: return "Line shader with color"
        case .lineGroupShader: return "Line Group shader"
        case .polygonGroupShader: return "Polygon Group shader"
        case .pointShader: return "Point (round) shader with color"
        case .colorShader: return "Color shader"
        case .roundColorShader: return "Round color shader"
        case .clearStencilShader: return "Clear stencil shader"
        case .textShader: return "Text shader"
        }
    }

    var vertexShader: String {
        switch self {
        case .alphaShader: return "baseVertexShader"
        case .lineShader: return "lineVertexShader"
        case .lineGroupShader: return "lineGroupVertexShader"
        case .polygonGroupShader: return "polygonGroupVertexShader"
        case .pointShader: return "pointVertexShader"
        case .colorShader: return "colorVertexShader"
        case .roundColorShader: return "colorVertexShader"
        case .clearStencilShader: return "stencilClearVertexShader"
        case .textShader: return "textVertexShader"
        }
    }

    var fragmentShader: String {
        switch self {
        case .alphaShader: return "baseFragmentShader"
        case .lineShader: return "lineFragmentShader"
        case .lineGroupShader: return "lineGroupFragmentShader"
        case .polygonGroupShader: return "polygonGroupFragmentShader"
        case .pointShader: return "pointFragmentShader"
        case .colorShader: return "colorFragmentShader"
        case .roundColorShader: return "roundColorFragmentShader"
        case .clearStencilShader: return "stencilClearFragmentShader"
        case .textShader: return "textFragmentShader"
        }
    }

    var vertexDescriptor: MTLVertexDescriptor {
        switch self {
        case .lineShader: return LineVertex.descriptor
        case .lineGroupShader: return LineVertex.descriptor
        case .polygonGroupShader: return PolygonVertex.descriptor
        default: return Vertex.descriptor
        }
    }
}

public class PipelineLibrary: StaticMetalLibrary<String, MTLRenderPipelineState> {
    init(device: MTLDevice) throws {
        try super.init(Pipeline.allCases.map(\.rawValue)) { (key) -> MTLRenderPipelineState in
            let pipelineDescriptor = PipelineDescriptorFactory.pipelineDescriptor(pipeline: .init(rawValue: key)!)
            return try device.makeRenderPipelineState(descriptor: pipelineDescriptor)
        }
    }
}
