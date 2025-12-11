/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

@preconcurrency import Metal
import OSLog

public enum PipelineDescriptorFactory {
    public static func pipelineDescriptor(
        vertexDescriptor: MTLVertexDescriptor,
        label: String,
        vertexShader: String,
        fragmentShader: String,
        blendMode: MCBlendMode,
        library: MTLLibrary,
        constants: MTLFunctionConstantValues? = nil,
        tessellation: MCTessellationMode = MCTessellationMode.NONE
    ) -> MTLRenderPipelineDescriptor {
        let pipelineDescriptor = MTLRenderPipelineDescriptor()
        pipelineDescriptor.colorAttachments[0].pixelFormat = MetalContext.colorPixelFormat
        pipelineDescriptor.vertexDescriptor = vertexDescriptor

        pipelineDescriptor.rasterSampleCount = 1  // samples per pixel

        let renderbufferAttachment = pipelineDescriptor.colorAttachments[0]
        renderbufferAttachment?.pixelFormat = MetalContext.colorPixelFormat
        renderbufferAttachment?.isBlendingEnabled = true

        switch blendMode {
            case .NORMAL:
                renderbufferAttachment?.rgbBlendOperation = .add
                renderbufferAttachment?.alphaBlendOperation = .add
                renderbufferAttachment?.sourceRGBBlendFactor = .one
                renderbufferAttachment?.sourceAlphaBlendFactor = .one
                renderbufferAttachment?.destinationRGBBlendFactor = .oneMinusSourceAlpha
                renderbufferAttachment?.destinationAlphaBlendFactor = .oneMinusSourceAlpha

            case .MULTIPLY:
                renderbufferAttachment?.rgbBlendOperation = .add
                renderbufferAttachment?.alphaBlendOperation = .add
                renderbufferAttachment?.sourceRGBBlendFactor = .destinationColor
                renderbufferAttachment?.sourceAlphaBlendFactor = .destinationAlpha
                renderbufferAttachment?.destinationRGBBlendFactor = .oneMinusSourceAlpha
                renderbufferAttachment?.destinationAlphaBlendFactor = .oneMinusSourceAlpha

            @unknown default:
                fatalError("blendMode not implemented")
        }

        pipelineDescriptor.stencilAttachmentPixelFormat = .stencil8
        pipelineDescriptor.label = label

        if let constants = constants {
            guard let vertexFunction = try? library.makeFunction(name: vertexShader, constantValues: constants),
                let fragmentFunction = try? library.makeFunction(name: fragmentShader, constantValues: constants)
            else {
                fatalError("Cannot locate the shaders for \(label)")
            }

            pipelineDescriptor.vertexFunction = vertexFunction
            pipelineDescriptor.fragmentFunction = fragmentFunction
        } else {
            guard let vertexFunction = library.makeFunction(name: vertexShader),
                let fragmentFunction = library.makeFunction(name: fragmentShader)
            else {
                fatalError("Cannot locate the shaders for \(label)")
            }

            pipelineDescriptor.vertexFunction = vertexFunction
            pipelineDescriptor.fragmentFunction = fragmentFunction
        }
        
        if tessellation != MCTessellationMode.NONE {
            pipelineDescriptor.maxTessellationFactor = 64
            pipelineDescriptor.tessellationPartitionMode = .pow2
            pipelineDescriptor.tessellationFactorFormat = .half
            pipelineDescriptor.tessellationFactorStepFunction = .constant
            pipelineDescriptor.tessellationOutputWindingOrder = .clockwise
            pipelineDescriptor.tessellationControlPointIndexType = .none
            pipelineDescriptor.isTessellationFactorScaleEnabled = false
            
            if tessellation == MCTessellationMode.TRIANGLE {
                pipelineDescriptor.tessellationOutputWindingOrder = .counterClockwise
                pipelineDescriptor.tessellationControlPointIndexType = .uint16
            }
        }
        return pipelineDescriptor
    }
}

extension PipelineDescriptorFactory {
    static func pipelineDescriptor(pipeline: Pipeline, library: MTLLibrary) -> MTLRenderPipelineDescriptor {
        pipelineDescriptor(
            vertexDescriptor: pipeline.type.vertexDescriptor,
            label: pipeline.type.label,
            vertexShader: pipeline.type.vertexShader,
            fragmentShader: pipeline.type.fragmentShader,
            blendMode: pipeline.blendMode,
            library: library,
            tessellation: pipeline.type.tessellation
        )
    }
}

public struct Pipeline: Codable, CaseIterable, Hashable, Sendable {
    let type: PipelineType
    let blendMode: MCBlendMode

    init(type: PipelineType, blendMode: MCBlendMode) {
        self.type = type
        self.blendMode = blendMode
    }

    // Conform to `Hashable` by implementing the `hash(into:)` method
    public func hash(into hasher: inout Hasher) {
        hasher.combine(type)
        hasher.combine(blendMode)
    }

    // Conform to `Equatable` (needed for `Hashable`)
    public static func == (lhs: Pipeline, rhs: Pipeline) -> Bool {
        return lhs.type == rhs.type && lhs.blendMode == rhs.blendMode
    }

    public static var allCases: [Pipeline] {
        Array(
            PipelineType.allCases
                .map { type in
                    MCBlendMode.allCases.map { blendMode in
                        Pipeline(type: type, blendMode: blendMode)
                    }
                }
                .joined())
    }
}

public enum PipelineType: String, CaseIterable, Codable, Sendable {
    case alphaShader
    case alphaInstancedShader
    case lineGroupShader
    case simpleLineGroupShader
    case unitSphereSimpleLineGroupShader
    case unitSphereLineGroupShader
    case polygonGroupShader
    case polygonStripedGroupShader
    case polygonPatternGroupShader
    case polygonPatternFadeInGroupShader
    case maskShader
    case maskTessellatedShader
    case colorShader
    case polygonTessellatedShader
    case roundColorShader
    case clearStencilShader
    case textShader
    case textInstancedShader
    case rasterShader
    case quadTessellatedShader
    case stretchShader
    case stretchInstancedShader
    case unitSphereAlphaShader
    case unitSphereRoundColorShader
    case unitSphereAlphaInstancedShader
    case unitSphereTextInstancedShader
    case sphereEffectShader
    case skySphereShader
    case elevationInterpolation

    var label: String {
        switch self {
            case .alphaShader: return "Alpha shader with texture"
            case .alphaInstancedShader: return "Alpha instanced shader with texture"
            case .lineGroupShader: return "Line Group shader"
            case .unitSphereLineGroupShader: return "Unit sphere line Group shader"
            case .simpleLineGroupShader: return "Simple Line Group shader"
            case .unitSphereSimpleLineGroupShader: return "Unit sphere simple line Group shader"
            case .polygonGroupShader: return "Polygon Group shader"
            case .polygonStripedGroupShader: return "Polygon Group (striped) shader"
            case .polygonPatternGroupShader: return "Polygon Group Pattern shader"
            case .polygonPatternFadeInGroupShader: return "Polygon Group Pattern (fade in) shader"
            case .maskShader: return "Mask shader"
            case .maskTessellatedShader: return "Mask Tessellated shader"
            case .colorShader: return "Color shader"
            case .polygonTessellatedShader: return "Polygon Tessellated shader"
            case .roundColorShader: return "Round color shader"
            case .clearStencilShader: return "Clear stencil shader"
            case .textShader: return "Text shader"
            case .textInstancedShader: return "Text Instanced shader"
            case .rasterShader: return "Raster shader"
            case .quadTessellatedShader: return "Quad Tessellated shader"
            case .stretchShader: return "Stretch shader"
            case .stretchInstancedShader: return "Stretch Instanced shader"
            case .unitSphereAlphaShader: return "Unit Sphere Alpha shader with texture"
            case .unitSphereRoundColorShader: return "Unit Sphere Round color shader"
            case .unitSphereAlphaInstancedShader: return "Unit Sphere Alpha instanced shader with texture"
            case .unitSphereTextInstancedShader: return "Unit Sphere Text Instanced shader"
            case .sphereEffectShader: return "Sphere Effect Shader"
            case .skySphereShader: return "Sky Effect Shader"
            case .elevationInterpolation: return "Elevation Interpolation"
        }
    }

    var vertexShaderUsesModelMatrix: Bool {
        switch self {
            case .rasterShader, .quadTessellatedShader, .roundColorShader, .unitSphereRoundColorShader, .alphaShader, .unitSphereAlphaShader, .sphereEffectShader, .skySphereShader, .elevationInterpolation:
                return true
            default:
                return false
        }
    }

    var vertexShader: String {
        switch self {
            case .alphaShader: return "baseVertexShaderModel"
            case .alphaInstancedShader: return "alphaInstancedVertexShader"
            case .lineGroupShader: return "lineGroupVertexShader"
            case .unitSphereLineGroupShader: return "unitSphereLineGroupVertexShader"
            case .simpleLineGroupShader: return "simpleLineGroupVertexShader"
            case .unitSphereSimpleLineGroupShader: return "unitSphereSimpleLineGroupVertexShader"
            case .polygonGroupShader: return "polygonGroupVertexShader"
            case .polygonStripedGroupShader: return "polygonStripedGroupVertexShader"
            case .polygonPatternGroupShader: return "polygonPatternGroupVertexShader"
            case .polygonPatternFadeInGroupShader: return "polygonPatternGroupVertexShader"
            case .maskShader: return "colorVertexShader"
            case .maskTessellatedShader: return "polygonTessellationVertexShader"
            case .colorShader: return "colorVertexShader"
            case .polygonTessellatedShader: return "polygonTessellationVertexShader"
            case .roundColorShader: return "baseVertexShaderModel"
            case .clearStencilShader: return "stencilClearVertexShader"
            case .textShader: return "textVertexShader"
            case .textInstancedShader: return "textInstancedVertexShader"
            case .rasterShader: return "baseVertexShaderModel"
            case .quadTessellatedShader: return "quadTessellationVertexShader"
            case .stretchShader: return "stretchVertexShader"
            case .stretchInstancedShader: return "stretchInstancedVertexShader"
            case .unitSphereAlphaShader: return "baseVertexShader"
            case .unitSphereRoundColorShader: return "baseVertexShaderModel"
            case .unitSphereAlphaInstancedShader: return "unitSphereAlphaInstancedVertexShader"
            case .unitSphereTextInstancedShader: return "unitSphereTextInstancedVertexShader"
            case .sphereEffectShader: return "baseVertexShader"
            case .skySphereShader: return "baseVertexShader"
            case .elevationInterpolation: return "baseVertexShaderModel"
        }
    }

    var fragmentShader: String {
        switch self {
            case .alphaShader: return "baseFragmentShader"
            case .alphaInstancedShader: return "alphaInstancedFragmentShader"
            case .lineGroupShader: return "lineGroupFragmentShader"
            case .unitSphereLineGroupShader: return "lineGroupFragmentShader"
            case .simpleLineGroupShader: return "simpleLineGroupFragmentShader"
            case .unitSphereSimpleLineGroupShader: return "simpleLineGroupFragmentShader"
            case .polygonGroupShader: return "polygonGroupFragmentShader"
            case .polygonStripedGroupShader: return "polygonGroupStripedFragmentShader"
            case .polygonPatternGroupShader: return "polygonPatternGroupFragmentShader"
            case .polygonPatternFadeInGroupShader: return "polygonPatternGroupFadeInFragmentShader"
            case .maskShader: return "maskFragmentShader"
            case .maskTessellatedShader: return "maskFragmentShader"
            case .colorShader: return "colorFragmentShader"
            case .polygonTessellatedShader: return "colorFragmentShader"
            case .roundColorShader: return "roundColorFragmentShader"
            case .clearStencilShader: return "stencilClearFragmentShader"
            case .textShader: return "textFragmentShader"
            case .textInstancedShader: return "textInstancedFragmentShader"
            case .rasterShader: return "rasterFragmentShader"
            case .quadTessellatedShader: return "rasterFragmentShader"
            case .stretchShader: return "stretchFragmentShader"
            case .stretchInstancedShader: return "stretchInstancedFragmentShader"
            case .unitSphereAlphaShader: return "baseFragmentShader"
            case .unitSphereRoundColorShader: return "roundColorFragmentShader"
            case .unitSphereAlphaInstancedShader: return "unitSphereAlphaInstancedFragmentShader"
            case .unitSphereTextInstancedShader: return "unitSphereTextInstancedFragmentShader"
            case .sphereEffectShader: return "sphereEffectFragmentShader"
            case .skySphereShader: return "skySphereFragmentShader"
            case .elevationInterpolation: return "elevationInterpolationFragmentShader"
        }
    }

    var vertexDescriptor: MTLVertexDescriptor {
        switch self {
            case .lineGroupShader, .simpleLineGroupShader:
                return LineVertex.descriptor
            case .unitSphereLineGroupShader, .unitSphereSimpleLineGroupShader:
                return LineVertex.descriptorUnitSphere
            case .polygonGroupShader,
                .polygonPatternGroupShader,
                .polygonPatternFadeInGroupShader,
                .polygonStripedGroupShader,
                .colorShader, .maskShader:
                return Vertex3D.descriptor
            case .rasterShader,
                .clearStencilShader,
                .alphaShader,
                .unitSphereAlphaShader,
                .unitSphereRoundColorShader,
                .sphereEffectShader,
                .skySphereShader,
                .roundColorShader,
                .elevationInterpolation:
                return Vertex3DTexture.descriptor
            case .maskTessellatedShader,
                .polygonTessellatedShader:
                return Vertex3DTessellated.descriptor
            case .quadTessellatedShader:
                return Vertex3DTextureTessellated.descriptor
            default:
                return Vertex.descriptor
        }
    }
    
    var tessellation: MCTessellationMode {
        switch self {
            case .quadTessellatedShader:
                return MCTessellationMode.QUAD
            case .maskTessellatedShader,
                .polygonTessellatedShader:
               return MCTessellationMode.TRIANGLE
            default:
                return MCTessellationMode.NONE
        }
    }
}

public typealias PipelineLibrary = StaticMetalLibrary<Pipeline, MTLRenderPipelineState>

extension PipelineLibrary {
    init(device: MTLDevice, library: MTLLibrary) throws {
        try self
            .init(
                Pipeline.allCases.map(\.self)
            ) { pipeline -> MTLRenderPipelineState in
                do {
                    let pipelineDescriptor = PipelineDescriptorFactory.pipelineDescriptor(pipeline: pipeline, library: library)
                    return try device.makeRenderPipelineState(descriptor: pipelineDescriptor)
                } catch {
                    // Log the JSON (key) and the error
                    Logger().error("Error creating pipeline for: \(pipeline.type.rawValue, privacy: .public), \(pipeline.blendMode.rawValue, privacy: .public) error: \(error, privacy: .public)")
                    throw error
                }
            }
    }
}

extension MCBlendMode: Codable, @retroactive CaseIterable {
    public static var allCases: [MCBlendMode] {
        [.NORMAL, .MULTIPLY]
    }
}
