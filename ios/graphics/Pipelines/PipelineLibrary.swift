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

    public enum BlendMode {
        case normal
        case multiply
        case add
    }

    public static func pipelineDescriptor(vertexDescriptor: MTLVertexDescriptor,
                                          label: String,
                                          vertexShader: String,
                                          fragmentShader: String,
                                          library: MTLLibrary = MetalContext.current.library,
                                          blendMode: BlendMode = .normal,
                                          tessellation: Bool = false
    ) -> MTLRenderPipelineDescriptor {
        let pipelineDescriptor = MTLRenderPipelineDescriptor()
        pipelineDescriptor.colorAttachments[0].pixelFormat = MetalContext.colorPixelFormat
        pipelineDescriptor.vertexDescriptor = vertexDescriptor

        let renderbufferAttachment = pipelineDescriptor.colorAttachments[0]
        renderbufferAttachment?.pixelFormat = MetalContext.colorPixelFormat
        renderbufferAttachment?.isBlendingEnabled = true
        renderbufferAttachment?.rgbBlendOperation = .add
        renderbufferAttachment?.alphaBlendOperation = .add
        renderbufferAttachment?.sourceRGBBlendFactor = .one
        renderbufferAttachment?.sourceAlphaBlendFactor = .one
        renderbufferAttachment?.destinationRGBBlendFactor = .oneMinusSourceAlpha
        renderbufferAttachment?.destinationAlphaBlendFactor = .oneMinusSourceAlpha

        switch blendMode {
            case .normal: break // keep defaults
            case .multiply:
                renderbufferAttachment?.sourceRGBBlendFactor = .zero
                renderbufferAttachment?.sourceAlphaBlendFactor = .zero
                renderbufferAttachment?.destinationRGBBlendFactor = .sourceColor
                renderbufferAttachment?.destinationAlphaBlendFactor = .sourceAlpha
            case .add:
                renderbufferAttachment?.sourceRGBBlendFactor = .one
                renderbufferAttachment?.sourceAlphaBlendFactor = .one
                renderbufferAttachment?.destinationRGBBlendFactor = .one
                renderbufferAttachment?.destinationAlphaBlendFactor = .one

        }

        if tessellation {
            pipelineDescriptor.isTessellationFactorScaleEnabled = false
            pipelineDescriptor.tessellationFactorFormat = .half
            pipelineDescriptor.tessellationControlPointIndexType = .uint16
            pipelineDescriptor.tessellationFactorStepFunction = .constant
            pipelineDescriptor.maxTessellationFactor = 16
        }

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
        pipelineDescriptor(vertexDescriptor: pipeline.vertexDescriptor,
                           label: pipeline.label,
                           vertexShader: pipeline.vertexShader,
                           fragmentShader: pipeline.fragmentShader,
                           tessellation: pipeline.tessellation)
    }
}

public enum Pipeline: String, CaseIterable {
    case alphaShader
    case lineGroupShader
    case polygonGroupShader
    case pointShader
    case colorShader
    case roundColorShader
    case clearStencilShader
    case textShader
    case combineLayersShader

    var label: String {
        switch self {
            case .alphaShader: return "Alpha shader with texture"
            case .lineGroupShader: return "Line Group shader"
            case .polygonGroupShader: return "Polygon Group shader"
            case .pointShader: return "Point (round) shader with color"
            case .colorShader: return "Color shader"
            case .roundColorShader: return "Round color shader"
            case .clearStencilShader: return "Clear stencil shader"
            case .textShader: return "Text shader"
            case .combineLayersShader: return "Combine Layers Shader"
        }
    }

    var vertexShader: String {
        switch self {
            case .alphaShader: return "baseVertexShader"
            case .lineGroupShader: return "lineGroupVertexShader"
            case .polygonGroupShader: return "polygonGroupVertexShader"
            case .pointShader: return "pointVertexShader"
            case .colorShader: return "colorVertexShader"
            case .roundColorShader: return "colorVertexShader"
            case .clearStencilShader: return "stencilClearVertexShader"
            case .textShader: return "textVertexShader"
            case .combineLayersShader: return "flatBaseVertexShader"
        }
    }

    var fragmentShader: String {
        switch self {
            case .alphaShader: return "baseFragmentShader"
            case .lineGroupShader: return "lineGroupFragmentShader"
            case .polygonGroupShader: return "polygonGroupFragmentShader"
            case .pointShader: return "pointFragmentShader"
            case .colorShader: return "colorFragmentShader"
            case .roundColorShader: return "roundColorFragmentShader"
            case .clearStencilShader: return "stencilClearFragmentShader"
            case .textShader: return "textFragmentShader"
            case .combineLayersShader: return "baseFragmentShader"
        }
    }

    var tessellation: Bool {
        switch self {
            case .colorShader: return true
            case .alphaShader: return true
            case .roundColorShader: return true
            default: return false
        }
    }

    var vertexDescriptor: MTLVertexDescriptor {
        switch self {
            case .lineGroupShader: return LineVertex.descriptor
            case .polygonGroupShader: return PolygonVertex.descriptor
            default:
                if tessellation {
                    return Vertex.tesselatedDescriptor
                }
                else {
                    return Vertex.descriptor
                }
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
            do {
                return try device.makeRenderPipelineState(descriptor: pipelineDescriptor)
            }
            catch {
                print("failed to create Pipeline Descriptor for \(key)", error)
                throw error
            }
        }
    }
}
