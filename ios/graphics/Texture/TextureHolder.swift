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
    public let texture: MTLTexture

    let textureUsableSize: TextureUsableSize?

    init(_ texture: MTLTexture, textureUsableSize: TextureUsableSize? = nil) {
        self.texture = texture
        self.textureUsableSize = textureUsableSize
        super.init()
    }

    deinit {
        let texture = self.texture
        DispatchQueue.global(qos: .utility).async {
            // access texture to make sure it gets deallocated asynchron
            _ = texture.width
        }
    }

    public convenience init(_ url: URL, textureUsableSize: TextureUsableSize? = nil) throws {
        let options: [MTKTextureLoader.Option: Any] = [
            MTKTextureLoader.Option.SRGB: NSNumber(booleanLiteral: false),
        ]
        let texture = try MetalContext.current.textureLoader.newTexture(URL: url, options: options)
        self.init(texture, textureUsableSize: textureUsableSize)
    }

    public convenience init(_ cgImage: CGImage) throws {
        let options: [MTKTextureLoader.Option: Any] = [
            MTKTextureLoader.Option.SRGB: NSNumber(booleanLiteral: false),
            MTKTextureLoader.Option.textureStorageMode: MTLStorageMode.shared.rawValue,
        ]

        do {
            let texture = try MetalContext.current.textureLoader.newTexture(cgImage: cgImage, options: options)

            self.init(texture)
        } catch {
            guard let fixedImage = UIImage(cgImage: cgImage).ub_metalFixMe().cgImage else {
                throw error
            }

            let texture = try MetalContext.current.textureLoader.newTexture(cgImage: fixedImage, options: options)

            self.init(texture)
        }
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

    public convenience init(_ size: CGSize, drawCallback: (CGContext) -> Void) throws {
        guard size.width > 0, size.height > 0 else {
            throw TextureHolderError.emptyData
        }

        let width: Int = Int(size.width)
        let height: Int = Int(size.height)

        let bytesPerPixel = 4
        let bytesPerRow = bytesPerPixel * width

        let td = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .rgba8Unorm, width: width, height: height, mipmapped: false)
        td.usage = .shaderRead

        guard let texture = MetalContext.current.device.makeTexture(descriptor: td) else {
            throw TextureHolderError.emptyData
        }

        let length = 4 * width * height
        guard let imageData = calloc(width * height, 4) else {
            throw TextureHolderError.emptyData
        }

        let data = NSMutableData(bytesNoCopy: imageData, length: length, freeWhenDone: true)

        let colorSpace = CGColorSpaceCreateDeviceRGB()
        let bitmapInfo = CGBitmapInfo(rawValue: CGBitmapInfo.byteOrder32Big.rawValue | CGImageAlphaInfo.premultipliedLast.rawValue)

        guard let context = CGContext(data: data.mutableBytes, width: width, height: height, bitsPerComponent: 8, bytesPerRow: 4 * width, space: colorSpace, bitmapInfo: bitmapInfo.rawValue) else {
            throw TextureHolderError.emptyData
        }

        context.translateBy(x: 0, y: size.height)
        context.scaleBy(x: 1.0, y: -1.0)

        UIGraphicsPushContext(context)
        drawCallback(context)
        UIGraphicsPopContext()

        let region = MTLRegionMake2D(0, 0, width, height)
        texture.replace(region: region, mipmapLevel: 0, withBytes: data.bytes, bytesPerRow: bytesPerRow)

        self.init(texture, textureUsableSize: nil)
    }
}

extension TextureHolder: MCTextureHolderInterface {
    public func clearFromGraphics() {
    }

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
