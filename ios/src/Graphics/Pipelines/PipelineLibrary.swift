//
//  PipelineLibrary.swift
//  SwisstopoShared
//
//  Created by Joseph El Mallah on 13.02.20.
//  Copyright © 2020 Ubique Innovation AG. All rights reserved.
//

import Metal

enum PipelineKey: CaseIterable {
    case alphaShader
    case lineShader
    case pointShader
    case colorShader
    case roundColorShader

    fileprivate func pipelineDescriptor() -> MTLRenderPipelineDescriptor {
        let pipelineDescriptor = MTLRenderPipelineDescriptor()
        pipelineDescriptor.colorAttachments[0].pixelFormat = MetalContext.current.colorPixelFormat
        pipelineDescriptor.vertexDescriptor = Vertex.descriptor

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
        pipelineDescriptor.label = label()

        guard let vertexFunction = MetalContext.current.library.makeFunction(name: vertexShader()),
              let fragmentFunction = MetalContext.current.library.makeFunction(name: fragmentShader()) else {
            fatalError("Cannot locate the shaders for UBTileModel")
        }

        pipelineDescriptor.vertexFunction = vertexFunction
        pipelineDescriptor.fragmentFunction = fragmentFunction

        return pipelineDescriptor
    }

    private func label() -> String {
        switch self {
        case .alphaShader: return "Alpha shader with texture"
        case .lineShader: return "Line shader with color"
        case .pointShader: return "Point (round) shader with color"
        case .colorShader: return "Color shader"
        case .roundColorShader: return "Round color shader"
        }
    }

    private func vertexShader() -> String {
        switch self {
        case .alphaShader: return "baseVertexShader"
        case .lineShader: return "lineVertexShader"
        case .pointShader: return "pointVertexShader"
        case .colorShader: return "colorVertexShader"
        case .roundColorShader: return "colorVertexShader"
        }
    }

    private func fragmentShader() -> String {
        switch self {
        case .alphaShader: return "baseFragmentShader"
        case .lineShader: return "lineFragmentShader"
        case .pointShader: return "pointFragmentShader"
        case .colorShader: return "colorFragmentShader"
        case .roundColorShader: return "roundColorFragmentShader"
        }
    }
}

class PipelineLibrary: StaticMetalLibrary<PipelineKey, MTLRenderPipelineState> {
    init(device: MTLDevice) throws {
        try super.init { (key) -> MTLRenderPipelineState in
            let pipelineDescriptor = key.pipelineDescriptor()
            return try device.makeRenderPipelineState(descriptor: pipelineDescriptor)
        }
    }
}
