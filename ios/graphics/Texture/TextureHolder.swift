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

    var _texture: MTLTexture?
    var texture: MTLTexture? {
        get {
            if _texture == nil, textureSource != nil {
                try? loadDataFromSource()
            }
            return _texture
        }
        set {
            _texture = newValue
        }
    }
    var textureUsableSize: TextureUsableSize?
    var textureSource: TextureSource?

    enum TextureSource {
        case url(URL)
        case cgImage(CGImage)
        case name(String, scaleFactor: Double, bundle: Bundle?)
        case data(Data)
    }

    init(_ texture: MTLTexture, textureUsableSize: TextureUsableSize? = nil) {
        self._texture = texture
        self.textureUsableSize = textureUsableSize
        super.init()
    }

    init(_ url: URL, textureUsableSize: TextureUsableSize? = nil) throws {
        self.textureUsableSize = textureUsableSize
        self.textureSource = .url(url)
    }

    public init(_ cgImage: CGImage)  {
        self.textureSource = .cgImage(cgImage)
    }


    public init(name: String, scaleFactor: Double, bundle: Bundle?) throws {
        self.textureSource = .name(name, scaleFactor: scaleFactor, bundle: bundle)
    }

    public init(_ data: Data, textureUsableSize: TextureUsableSize? = nil) throws {
        guard !data.isEmpty else {
            throw TextureHolderError.emptyData
        }
        self.textureUsableSize = textureUsableSize
        self.textureSource = .data(data)
    }

    private func loadDataFromSource() throws {
        switch textureSource {
            case nil: break
            case .some(.url(let url)):
                let options: [MTKTextureLoader.Option: Any] = [
                    MTKTextureLoader.Option.SRGB: NSNumber(booleanLiteral: false),
                ]
                self.texture = try MetalContext.current.textureLoader.newTexture(URL: url, options: options)

            case .some(.cgImage(let cgImage)):
                let options: [MTKTextureLoader.Option: Any] = [
                    MTKTextureLoader.Option.SRGB: NSNumber(booleanLiteral: false),
                ]
                self.texture = try MetalContext.current.textureLoader.newTexture(cgImage: cgImage, options: options)

            case .some(.name(let name, scaleFactor: let scaleFactor, bundle: let bundle)):
                let options: [MTKTextureLoader.Option: Any] = [
                    MTKTextureLoader.Option.SRGB: NSNumber(booleanLiteral: false),
                ]
                self.texture = try MetalContext.current.textureLoader.newTexture(name: name, scaleFactor: scaleFactor, bundle: bundle, options: options)

            case .some(.data(let data)):
                guard !data.isEmpty else {
                    throw TextureHolderError.emptyData
                }
                let options: [MTKTextureLoader.Option: Any] = [
                    MTKTextureLoader.Option.SRGB: NSNumber(booleanLiteral: false),
                ]
                self.texture = try MetalContext.current.textureLoader.newTexture(data: data, options: options)

        }
    }

    func clearSource(loadTexture: Bool) throws {
        if loadTexture {
            try loadDataFromSource()
        }
        textureSource = nil
    }
}

extension TextureHolder: MCTextureHolderInterface {
    public func clearFromGraphics() {
        if textureSource != nil {
            // only clear texture, if it can be restored
            texture = nil
        }
    }

    public func attachToGraphics() -> Int32 {
        if _texture == nil {
            try! loadDataFromSource()
        }
        return 0
    }

    public func getImageWidth() -> Int32 {
        Int32(texture!.width) // TODO: Remove !
    }

    public func getImageHeight() -> Int32 {
        Int32(texture!.height) // TODO: Remove !
    }

    public func getTextureWidth() -> Int32 {
        Int32(textureUsableSize?.width ?? texture!.width) // TODO: Remove !
    }

    public func getTextureHeight() -> Int32 {
        Int32(textureUsableSize?.height ?? texture!.height) // TODO: Remove !
    }
}

public extension TextureHolder {
    struct TextureUsableSize {
        let width: Int
        let height: Int
    }
}
