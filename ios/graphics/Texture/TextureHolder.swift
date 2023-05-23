/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import Foundation
import MapCoreSharedModule
import MetalKit

enum TextureHolderError: Error {
    case emptyData
}

@objc
public class TextureHolder: NSObject {
    let texture: MTLTexture

    let textureUsableSize: TextureUsableSize?

    init(_ texture: MTLTexture, textureUsableSize: TextureUsableSize? = nil) {
        self.texture = texture
        self.textureUsableSize = textureUsableSize
        super.init()
    }

    convenience init(_ url: URL, textureUsableSize: TextureUsableSize? = nil) throws {
        let options: [MTKTextureLoader.Option: Any] = [
            MTKTextureLoader.Option.SRGB: NSNumber(booleanLiteral: false),
        ]
        let texture = try MetalContext.current.textureLoader.newTexture(URL: url, options: options)
        self.init(texture, textureUsableSize: textureUsableSize)
    }

    public convenience init(_ cgImage: CGImage) throws {
        let options: [MTKTextureLoader.Option: Any] = [
            MTKTextureLoader.Option.SRGB: NSNumber(booleanLiteral: false),
            MTKTextureLoader.Option.textureStorageMode: MTLStorageMode.shared.rawValue
        ]

        let texture = try MetalContext.current.textureLoader.newTexture(cgImage: cgImage, options: options)
        self.init(texture)
    }

    public convenience init(name: String, scaleFactor: Double, bundle: Bundle?) throws {
        let options: [MTKTextureLoader.Option: Any] = [
            MTKTextureLoader.Option.SRGB: NSNumber(booleanLiteral: false),
        ]
        let texture = try MetalContext.current.textureLoader.newTexture(name: name, scaleFactor: scaleFactor, bundle: bundle, options: options)
        self.init(texture)
    }

    public convenience init(_ data: Data, textureUsableSize: TextureUsableSize? = nil) throws {
        guard !data.isEmpty else {
            throw TextureHolderError.emptyData
        }
        let options: [MTKTextureLoader.Option: Any] = [
            MTKTextureLoader.Option.SRGB: NSNumber(booleanLiteral: false),
        ]
        let texture = try MetalContext.current.textureLoader.newTexture(data: data, options: options)
        self.init(texture, textureUsableSize: textureUsableSize)
    }
}

extension TextureHolder: MCTextureHolderInterface {
    public func clearFromGraphics() {}

    public func attachToGraphics() -> Int32 { 0 }

    public func getImageWidth() -> Int32 {
        Int32(texture.width)
    }

    public func getImageHeight() -> Int32 {
        Int32(texture.height)
    }

    public func getTextureWidth() -> Int32 {
        Int32(textureUsableSize?.width ?? texture.width)
    }

    public func getTextureHeight() -> Int32 {
        Int32(textureUsableSize?.height ?? texture.height)
    }
}

public extension TextureHolder {
    struct TextureUsableSize {
        let width: Int
        let height: Int
    }
}
