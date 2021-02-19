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

@objc
public class TextureHolder: NSObject {
    let texture: MTLTexture?

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
        ]
        let texture = try MetalContext.current.textureLoader.newTexture(cgImage: cgImage, options: options)
        self.init(texture)
    }

    public convenience init(_ data: Data, textureUsableSize: TextureUsableSize? = nil) throws {
        let options: [MTKTextureLoader.Option: Any] = [
            MTKTextureLoader.Option.SRGB: NSNumber(booleanLiteral: false),
        ]
        let texture = try MetalContext.current.textureLoader.newTexture(data: data, options: options)
        self.init(texture, textureUsableSize: textureUsableSize)
    }
}

extension TextureHolder: MCTextureHolderInterface {
    public func getImageWidth() -> Int32 {
        Int32(texture?.width ?? 0)
    }

    public func getImageHeight() -> Int32 {
        Int32(texture?.height ?? 0)
    }

    public func getTextureWidth() -> Int32 {
        Int32(textureUsableSize?.width ?? texture?.width ?? 0)
    }

    public func getTextureHeight() -> Int32 {
        Int32(textureUsableSize?.height ?? texture?.height ?? 0)
    }

    public func attachToGraphics() {}
}

public extension TextureHolder {
    struct TextureUsableSize {
        let width: Int
        let height: Int
    }
}
