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
@preconcurrency import MetalKit

#if canImport(UIKit)
    import UIKit
#elseif canImport(AppKit) && !canImport(UIKit)
    import AppKit
#endif

enum TextureHolderError: Error {
    case emptyData
}

@objc
public class TextureHolder: NSObject, @unchecked Sendable {
    public let texture: MTLTexture

    let textureUsableSize: TextureUsableSize?

    init(_ texture: MTLTexture, textureUsableSize: TextureUsableSize? = nil) {
        self.texture = texture
        self.textureUsableSize = textureUsableSize
        super.init()
    }

    deinit {
        let texture = self.texture
        DispatchQueue.global(qos: .utility)
            .async {
                // access texture to make sure it gets deallocated asynchron
                _ = texture.width
            }
    }

    public convenience init(_ url: URL, textureUsableSize: TextureUsableSize? = nil) throws {
        let options: [MTKTextureLoader.Option: Any] = [
            MTKTextureLoader.Option.SRGB: NSNumber(booleanLiteral: false)
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
            #if canImport(UIKit)
                guard let fixedImage = UIImage(cgImage: cgImage).ub_metalFixMe().cgImage else {
                    throw error
                }

                let texture = try MetalContext.current.textureLoader.newTexture(cgImage: fixedImage, options: options)

                self.init(texture)
            #else
                throw error
            #endif
        }
    }

    public convenience init(name: String, scaleFactor: Double, bundle: Bundle?) throws {
        let options: [MTKTextureLoader.Option: Any] = [
            MTKTextureLoader.Option.SRGB: NSNumber(booleanLiteral: false)
        ]
        let texture = try MetalContext.current.textureLoader.newTexture(name: name, scaleFactor: scaleFactor, bundle: bundle, options: options)
        self.init(texture)
    }

    public convenience init(_ data: Data, textureUsableSize: TextureUsableSize? = nil) throws {
        guard !data.isEmpty else {
            throw TextureHolderError.emptyData
        }
        let options: [MTKTextureLoader.Option: Any] = [
            MTKTextureLoader.Option.SRGB: false
        ]
        let texture = try MetalContext.current.textureLoader.newTexture(data: data, options: options)
        self.init(texture, textureUsableSize: textureUsableSize)
    }

    public convenience init(premultipliedData data: Data, textureUsableSize: TextureUsableSize?) throws {
        guard !data.isEmpty else {
            throw TextureHolderError.emptyData
        }
        let texture = try TextureHolder.makePremultipliedTexture(from: data)
        self.init(texture, textureUsableSize: textureUsableSize)
    }

    public convenience init(_ data: Data, textureUsableSize: TextureUsableSize? = nil, premultipliedData: Bool) throws {
        guard !data.isEmpty else {
            throw TextureHolderError.emptyData
        }
        let texture: MTLTexture
        if premultipliedData {
            texture = try TextureHolder.makePremultipliedTexture(from: data)
        } else {
            texture = try TextureHolder.makeNonPremultipliedTexture(from: data)
        }
        self.init(texture, textureUsableSize: textureUsableSize)
    }

    private static func makePremultipliedTexture(from data: Data) throws -> MTLTexture {
        guard let cgImage = TextureHolder.makeCGImage(from: data) else {
            let options: [MTKTextureLoader.Option: Any] = [
                MTKTextureLoader.Option.SRGB: false
            ]
            return try MetalContext.current.textureLoader.newTexture(data: data, options: options)
        }
        return try TextureHolder.makePremultipliedTexture(from: cgImage)
    }

    private static func makePremultipliedTexture(from cgImage: CGImage) throws -> MTLTexture {
        let width = cgImage.width
        let height = cgImage.height
        let bytesPerPixel = 4
        let bytesPerRow = bytesPerPixel * width
        let colorSpace = CGColorSpaceCreateDeviceRGB()
        let bitmapInfo: UInt32 =
            CGImageAlphaInfo.premultipliedLast.rawValue
            | CGBitmapInfo.byteOrder32Big.rawValue
        guard
            let context = CGContext(
                data: nil,
                width: width,
                height: height,
                bitsPerComponent: 8,
                bytesPerRow: bytesPerRow,
                space: colorSpace,
                bitmapInfo: bitmapInfo
            )
        else {
            throw TextureHolderError.emptyData
        }
        context.draw(cgImage, in: CGRect(x: 0, y: 0, width: width, height: height))
        guard let bytes = context.data else {
            throw TextureHolderError.emptyData
        }
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: .rgba8Unorm,
            width: width,
            height: height,
            mipmapped: false
        )
        descriptor.usage = [.shaderRead]
        descriptor.storageMode = .shared
        guard let texture = MetalContext.current.device.makeTexture(descriptor: descriptor) else {
            throw TextureHolderError.emptyData
        }
        texture.replace(
            region: MTLRegionMake2D(0, 0, width, height),
            mipmapLevel: 0,
            withBytes: bytes,
            bytesPerRow: bytesPerRow
        )
        return texture
    }

    private static func makeNonPremultipliedTexture(from data: Data) throws -> MTLTexture {
        guard let cgImage = TextureHolder.makeCGImage(from: data) else {
            let options: [MTKTextureLoader.Option: Any] = [
                MTKTextureLoader.Option.SRGB: false
            ]
            return try MetalContext.current.textureLoader.newTexture(data: data, options: options)
        }
        return try TextureHolder.makeNonPremultipliedTexture(from: cgImage)
    }

    private static func makeCGImage(from data: Data) -> CGImage? {
        #if canImport(UIKit)
            return UIImage(data: data)?.cgImage
        #elseif canImport(AppKit) && !canImport(UIKit)
            return NSImage(data: data)?.mcCgImage
        #else
            return nil
        #endif
    }

    private static func makeNonPremultipliedTexture(from cgImage: CGImage) throws -> MTLTexture {
        let width = cgImage.width
        let height = cgImage.height
        let bytesPerPixel = 4
        let bytesPerRow = bytesPerPixel * width
        let colorSpace = CGColorSpaceCreateDeviceRGB()
        let bitmapInfo: UInt32 =
            CGImageAlphaInfo.premultipliedLast.rawValue
            | CGBitmapInfo.byteOrder32Big.rawValue
        guard
            let context = CGContext(
                data: nil,
                width: width,
                height: height,
                bitsPerComponent: 8,
                bytesPerRow: bytesPerRow,
                space: colorSpace,
                bitmapInfo: bitmapInfo
            )
        else {
            throw TextureHolderError.emptyData
        }
        context.draw(cgImage, in: CGRect(x: 0, y: 0, width: width, height: height))
        guard let bytes = context.data else {
            throw TextureHolderError.emptyData
        }
        TextureHolder.unpremultiplyAlpha(pixels: bytes.assumingMemoryBound(to: UInt8.self), totalPixels: width * height)
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: .rgba8Unorm,
            width: width,
            height: height,
            mipmapped: false
        )
        descriptor.usage = [.shaderRead]
        descriptor.storageMode = .shared
        guard let texture = MetalContext.current.device.makeTexture(descriptor: descriptor) else {
            throw TextureHolderError.emptyData
        }
        texture.replace(
            region: MTLRegionMake2D(0, 0, width, height),
            mipmapLevel: 0,
            withBytes: bytes,
            bytesPerRow: bytesPerRow
        )
        return texture
    }

    private static func unpremultiplyAlpha(pixels: UnsafeMutablePointer<UInt8>, totalPixels: Int) {
        for pixelIndex in 0..<totalPixels {
            let byteOffset = pixelIndex * 4
            let alpha = pixels[byteOffset + 3]
            if alpha != 0 && alpha != 255 {
                let multiplier = 255.0 / Float(alpha)
                pixels[byteOffset] = UInt8(min(255.0, Float(pixels[byteOffset]) * multiplier))
                pixels[byteOffset + 1] = UInt8(min(255.0, Float(pixels[byteOffset + 1]) * multiplier))
                pixels[byteOffset + 2] = UInt8(min(255.0, Float(pixels[byteOffset + 2]) * multiplier))
            }
        }
    }

    // Non-throwing failable wrapper exposed to Kotlin/Native's swiftPMImport so KMP callers
    // (e.g. fluid-meteogram's animation bitmap decoding) can construct a TextureHolder without
    // going through an Obj-C bridge class. Swallows the throw and returns `nil` on failure.
    @objc(initWithData:)
    public convenience init?(data: Data) {
        do {
            try self.init(data, textureUsableSize: nil)
        } catch {
            return nil
        }
    }

    @objc(initWithPremultipliedData:)
    public convenience init?(premultipliedData data: Data) {
        do {
            try self.init(premultipliedData: data, textureUsableSize: nil)
        } catch {
            return nil
        }
    }

    @objc(initWithBitmapData:width:height:)
    public convenience init?(bitmapData data: Data, width: Int, height: Int) {
        let bitmapInfo = CGBitmapInfo(
            rawValue: CGBitmapInfo.byteOrder32Big.rawValue | CGImageAlphaInfo.premultipliedLast.rawValue
        )
        guard
            let provider = CGDataProvider(data: data as CFData),
            let cgImage = CGImage(
                width: width,
                height: height,
                bitsPerComponent: 8,
                bitsPerPixel: 32,
                bytesPerRow: width * 4,
                space: CGColorSpaceCreateDeviceRGB(),
                bitmapInfo: bitmapInfo,
                provider: provider,
                decode: nil,
                shouldInterpolate: false,
                intent: .defaultIntent
            )
        else {
            return nil
        }
        do {
            try self.init(cgImage)
        } catch {
            return nil
        }
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

        #if canImport(UIKit)
            UIGraphicsPushContext(context)
            drawCallback(context)
            UIGraphicsPopContext()
        #elseif canImport(AppKit) && !canImport(UIKit)
            let graphicsContext = NSGraphicsContext(cgContext: context, flipped: false)
            NSGraphicsContext.saveGraphicsState()
            NSGraphicsContext.current = graphicsContext
            drawCallback(context)
            NSGraphicsContext.restoreGraphicsState()
        #else
            drawCallback(context)
        #endif

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
