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
                                          library: MTLLibrary = MetalContext.current.library,
                                          tessellation: Bool,
                                          depthBuffer: Bool
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

        if tessellation {
            pipelineDescriptor.isTessellationFactorScaleEnabled = false
            pipelineDescriptor.tessellationFactorFormat = .half
            pipelineDescriptor.tessellationControlPointIndexType = .uint16
            pipelineDescriptor.tessellationFactorStepFunction = .constant
            pipelineDescriptor.maxTessellationFactor = 64
        }
        if depthBuffer {
            pipelineDescriptor.depthAttachmentPixelFormat = .depth32Float_stencil8
            pipelineDescriptor.stencilAttachmentPixelFormat = .depth32Float_stencil8
        }
        else {
            pipelineDescriptor.stencilAttachmentPixelFormat = .stencil8
            pipelineDescriptor.depthAttachmentPixelFormat = .invalid
        }

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
    static func pipelineDescriptor(pipeline: Pipeline, depth: Bool) -> MTLRenderPipelineDescriptor {
        pipelineDescriptor(vertexDescriptor: pipeline.vertexDescriptor,
                           label: pipeline.label,
                           vertexShader: pipeline.vertexShader,
                           fragmentShader: pipeline.fragmentShader,
                           tessellation: pipeline.isTesselated,
                           depthBuffer: depth)
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
    case sphereProjectionShader
    case sphereColorShader
    case skyboxShader

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
            case .sphereProjectionShader: return "Sphere projection shader"
            case .sphereColorShader: return "Sphere Mask Shader"
            case .skyboxShader: return "Skybox Shader"
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
            case .sphereProjectionShader: return "sphereProjectionVertexShader"
            case .sphereColorShader: return "sphereColorVertexShader"
            case .skyboxShader: return "baseVertexShader"
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
            case .sphereProjectionShader: return "shadedFragmentShader"
            case .sphereColorShader: return "colorFragmentShader"
            case .skyboxShader: return "skyboxFragmentShader"
        }
    }

    var vertexDescriptor: MTLVertexDescriptor {
        switch self {
            case .lineGroupShader: return LineVertex.descriptor
            case .polygonGroupShader: return PolygonVertex.descriptor
            case .sphereProjectionShader: return Vertex.tesselatedDescriptor
            case .sphereColorShader: return Vertex.tesselatedDescriptor
            default: return Vertex.descriptor
        }
    }

    var isTesselated: Bool {
        switch self {
            case .sphereProjectionShader: return true
            case .sphereColorShader: return true
            default: return false
        }
    }

}

public class PipelineLibrary: StaticMetalLibrary<String, MTLRenderPipelineState> {
    init(device: MTLDevice, depth: Bool) throws {
        try super.init(Pipeline.allCases.map(\.rawValue)) { key -> MTLRenderPipelineState in
            guard let pipeline = Pipeline(rawValue: key) else {
                throw LibraryError.invalidKey
            }
            let pipelineDescriptor = PipelineDescriptorFactory.pipelineDescriptor(pipeline: pipeline, depth: depth)
            return try device.makeRenderPipelineState(descriptor: pipelineDescriptor)
        }
    }
}
