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

enum PipelineKey: CaseIterable {
    case alphaShader
    case lineShader
    case pointShader
    case colorShader
    case roundColorShader
    case clearStencilShader

    fileprivate func pipelineDescriptor() -> MTLRenderPipelineDescriptor {
        let pipelineDescriptor = MTLRenderPipelineDescriptor()
        pipelineDescriptor.colorAttachments[0].pixelFormat = MetalContext.current.colorPixelFormat
        pipelineDescriptor.vertexDescriptor = vertexDescriptor()

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
              let fragmentFunction = MetalContext.current.library.makeFunction(name: fragmentShader())
        else {
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
        case .clearStencilShader: return "Clear stencil shader"
        }
    }

    private func vertexShader() -> String {
        switch self {
        case .alphaShader: return "baseVertexShader"
        case .lineShader: return "lineVertexShader"
        case .pointShader: return "pointVertexShader"
        case .colorShader: return "colorVertexShader"
        case .roundColorShader: return "colorVertexShader"
        case .clearStencilShader: return "stencilClearVertexShader"
        }
    }

    private func fragmentShader() -> String {
        switch self {
        case .alphaShader: return "baseFragmentShader"
        case .lineShader: return "lineFragmentShader"
        case .pointShader: return "pointFragmentShader"
        case .colorShader: return "colorFragmentShader"
        case .roundColorShader: return "roundColorFragmentShader"
        case .clearStencilShader: return "stencilClearFragmentShader"
        }
    }

    private func vertexDescriptor() -> MTLVertexDescriptor {
        switch self {
        case .lineShader: return LineVertex.descriptor
        default: return Vertex.descriptor
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
