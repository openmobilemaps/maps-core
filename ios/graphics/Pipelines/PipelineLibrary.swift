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
import OSLog

public enum PipelineDescriptorFactory {
    public static func pipelineDescriptor(vertexDescriptor: MTLVertexDescriptor,
                                          label: String,
                                          vertexShader: String,
                                          fragmentShader: String,
                                          blendMode: MCBlendMode,
                                          library: MTLLibrary = MetalContext.current.library) -> MTLRenderPipelineDescriptor {
        let pipelineDescriptor = MTLRenderPipelineDescriptor()
        pipelineDescriptor.colorAttachments[0].pixelFormat = MetalContext.current.colorPixelFormat
        pipelineDescriptor.vertexDescriptor = vertexDescriptor

        pipelineDescriptor.rasterSampleCount = 1 // samples per pixel

        let renderbufferAttachment = pipelineDescriptor.colorAttachments[0]
        renderbufferAttachment?.pixelFormat = MetalContext.current.colorPixelFormat
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
        pipelineDescriptor(vertexDescriptor: pipeline.type.vertexDescriptor,
                           label: pipeline.type.label,
                           vertexShader: pipeline.type.vertexShader,
                           fragmentShader: pipeline.type.fragmentShader,
                           blendMode: pipeline.blendMode)
    }
}

public struct Pipeline: Codable, CaseIterable {
    let type: PipelineType
    let blendMode: MCBlendMode

    init(type: PipelineType, blendMode: MCBlendMode) {
        self.type = type
        self.blendMode = blendMode
    }

    public init?(json: String) {
        guard let data = json.data(using: .utf8),
              let obj = try? JSONDecoder().decode(Pipeline.self, from: data) else {
            return nil
        }
        self = obj
    }

    public static var allCases: [Pipeline] {
        Array(PipelineType.allCases.map { type in
            MCBlendMode.allCases.map { blendMode in
                Pipeline(type: type, blendMode: blendMode)
            }
        }.joined())
    }

    public var json: String {
        do {
            let encoder = JSONEncoder()
            encoder.outputFormatting = .sortedKeys

            let jsonData = try encoder.encode(self)

            guard let jsonString = String(data: jsonData, encoding: .utf8) else {
                fatalError("unable to encode Pipeline")
            }
            return jsonString
        } catch {
            fatalError("An error occurred while encoding Pipeline: \(error)")
        }
    }
}

public enum PipelineType: String, CaseIterable, Codable {
    case alphaShader
    case alphaInstancedShader
    case lineGroupShader
    case unitSphereLineGroupShader
    case polygonGroupShader
    case polygonStripedGroupShader
    case polygonPatternGroupShader
    case polygonPatternFadeInGroupShader
    case colorShader
    case roundColorShader
    case clearStencilShader
    case textShader
    case textInstancedShader
    case rasterShader
    case stretchShader
    case stretchInstancedShader
    case unitSphereAlphaShader
    case unitSphereRoundColorShader
    case unitSphereAlphaInstancedShader
    case unitSphereTextInstancedShader
    case sphereEffectShader

    var label: String {
        switch self {
            case .alphaShader: return "Alpha shader with texture"
            case .alphaInstancedShader: return "Alpha instanced shader with texture"
            case .lineGroupShader: return "Line Group shader"
            case .unitSphereLineGroupShader: return "Unit sphere line Group shader"
            case .polygonGroupShader: return "Polygon Group shader"
            case .polygonStripedGroupShader: return "Polygon Group (striped) shader"
            case .polygonPatternGroupShader: return "Polygon Group Pattern shader"
            case .polygonPatternFadeInGroupShader: return "Polygon Group Pattern (fade in) shader"
            case .colorShader: return "Color shader"
            case .roundColorShader: return "Round color shader"
            case .clearStencilShader: return "Clear stencil shader"
            case .textShader: return "Text shader"
            case .textInstancedShader: return "Text Instanced shader"
            case .rasterShader: return "Raster shader"
            case .stretchShader: return "Stretch shader"
            case .stretchInstancedShader: return "Stretch Instanced shader"
            case .unitSphereAlphaShader: return "Unit Sphere Alpha shader with texture"
            case .unitSphereRoundColorShader: return "Unit Sphere Round color shader"
            case .unitSphereAlphaInstancedShader: return "Unit Sphere Alpha instanced shader with texture"
            case .unitSphereTextInstancedShader: return "Unit Sphere Text Instanced shader"
            case .sphereEffectShader: return "Sphere Effect Shader"
        }
    }

    var vertexShader: String {
        switch self {
            case .alphaShader: return "baseVertexShader"
            case .alphaInstancedShader: return "alphaInstancedVertexShader"
            case .lineGroupShader: return "lineGroupVertexShader"
            case .unitSphereLineGroupShader: return "unitSpherelineGroupVertexShader"
            case .polygonGroupShader: return "polygonGroupVertexShader"
            case .polygonStripedGroupShader: return "polygonStripedGroupVertexShader"
            case .polygonPatternGroupShader: return "polygonPatternGroupVertexShader"
            case .polygonPatternFadeInGroupShader: return "polygonPatternGroupVertexShader"
            case .colorShader: return "colorVertexShader"
            case .roundColorShader: return "baseVertexShader"
            case .clearStencilShader: return "stencilClearVertexShader"
            case .textShader: return "textVertexShader"
            case .textInstancedShader: return "textInstancedVertexShader"
            case .rasterShader: return "baseVertexShader"
            case .stretchShader: return "stretchVertexShader"
            case .stretchInstancedShader: return "stretchInstancedVertexShader"
            case .unitSphereAlphaShader: return "baseVertexShader"
            case .unitSphereRoundColorShader: return "baseVertexShader"
            case .unitSphereAlphaInstancedShader: return "unitSphereAlphaInstancedVertexShader"
            case .unitSphereTextInstancedShader: return "unitSphereTextInstancedVertexShader"
            case .sphereEffectShader: return "baseVertexShader"
        }
    }

    var fragmentShader: String {
        switch self {
            case .alphaShader: return "baseFragmentShader"
            case .alphaInstancedShader: return "alphaInstancedFragmentShader"
            case .lineGroupShader: return "lineGroupFragmentShader"
            case .unitSphereLineGroupShader: return "lineGroupFragmentShader"
            case .polygonGroupShader: return "polygonGroupFragmentShader"
            case .polygonStripedGroupShader: return "polygonGroupStripedFragmentShader"
            case .polygonPatternGroupShader: return "polygonPatternGroupFragmentShader"
            case .polygonPatternFadeInGroupShader: return "polygonPatternGroupFadeInFragmentShader"
            case .colorShader: return "colorFragmentShader"
            case .roundColorShader: return "roundColorFragmentShader"
            case .clearStencilShader: return "stencilClearFragmentShader"
            case .textShader: return "textFragmentShader"
            case .textInstancedShader: return "textInstancedFragmentShader"
            case .rasterShader: return "rasterFragmentShader"
            case .stretchShader: return "stretchFragmentShader"
            case .stretchInstancedShader: return "stretchInstancedFragmentShader"
            case .unitSphereAlphaShader: return "baseFragmentShader"
            case .unitSphereRoundColorShader: return "roundColorFragmentShader"
            case .unitSphereAlphaInstancedShader: return "unitSphereAlphaInstancedFragmentShader"
            case .unitSphereTextInstancedShader: return "unitSphereTextInstancedFragmentShader"
            case .sphereEffectShader: return "sphereEffectFragmentShader"
        }
    }

    var vertexDescriptor: MTLVertexDescriptor {
        switch self {
            case .lineGroupShader:
                return LineVertex.descriptor
            case .unitSphereLineGroupShader:
                return LineVertex.descriptorUnitSphere
            case .polygonGroupShader,
                 .polygonPatternGroupShader,
                 .polygonPatternFadeInGroupShader,
                 .polygonStripedGroupShader:
                return Vertex4F.descriptor
            case .colorShader:
                return Vertex3F.descriptor
            case .rasterShader,
                 .clearStencilShader,
                 .alphaShader,
                 .unitSphereAlphaShader,
                 .unitSphereRoundColorShader,
                 .sphereEffectShader,
                 .roundColorShader:
                return Vertex3DTexture.descriptor
            default:
                return Vertex.descriptor
        }
    }

    var premultiplyAlpha: Bool {
        switch self {
            case .colorShader, .polygonGroupShader:
                return true
            default: return false
        }
    }
}

public class PipelineLibrary: StaticMetalLibrary<String, MTLRenderPipelineState>, @unchecked Sendable {
    init(device: MTLDevice) throws {
        try super.init(Pipeline.allCases.map(\.json)) { key -> MTLRenderPipelineState in
            do {
                guard let pipeline = Pipeline(json: key) else {
                    throw LibraryError.invalidKey
                }
                let pipelineDescriptor = PipelineDescriptorFactory.pipelineDescriptor(pipeline: pipeline)
                return try device.makeRenderPipelineState(descriptor: pipelineDescriptor)
            } catch {
                // Log the JSON (key) and the error
                Logger().error("Error creating pipeline for JSON: \(key, privacy: .public) error: \(error, privacy: .public)")
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
