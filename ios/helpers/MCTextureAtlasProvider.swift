//
//  MCTextureAtlasProvider.swift
//  MapCore
//
//  Created by Stefan Mitterrutzner on 30.05.2025.
//

import MapCoreSharedModule

#if canImport(UIKit)
    import UIKit
    public typealias PlatformImage = UIImage
#elseif canImport(AppKit)
    import AppKit
    public typealias PlatformImage = NSImage
#endif

public enum MCTextureAtlasProvider {

    public static func createTextureAtlas(
        images: [String: PlatformImage],
        maxAtlasSize: CGSize = CGSize(width: 4096, height: 4096)
    ) -> [MCTextureAtlas] {
        let packerResult = MCRectanglePacker.pack(
            images.mapValues {
                MCVec2I(x: Int32($0.size.width), y: Int32($0.size.height))
            },
            maxPageSize: .init(x: Int32(maxAtlasSize.width), y: Int32(maxAtlasSize.height))
        )

        return packerResult.compactMap { page -> MCTextureAtlas? in
            let sizes = page.uvs.values.map { CGSize(width: CGFloat($0.x + $0.width), height: CGFloat($0.y + $0.height)) }
            let maxWidth = sizes.map(\.width).max() ?? 0.0
            let maxHeight = sizes.map(\.height).max() ?? 0.0
            let canvasSize = CGSize(width: maxWidth, height: maxHeight)

            guard let combinedImage = drawImages(images: images, uvs: page.uvs, canvasSize: canvasSize) else {
                assertionFailure("Could not create image atlas")
                return nil
            }

            guard let cgImage = combinedImage.cgImage else {
                assertionFailure("Combined image has no CGImage representation")
                return nil
            }

            return MCTextureAtlas(
                uvMap: page.uvs,
                texture: try? TextureHolder(cgImage)
            )
        }
    }

    private static func drawImages(
        images: [String: PlatformImage],
        uvs: [String: MCRectI],
        canvasSize: CGSize
    ) -> PlatformImage? {
        #if canImport(UIKit)
            UIGraphicsBeginImageContextWithOptions(canvasSize, false, 1.0)
            defer { UIGraphicsEndImageContext() }

            for (key, rect) in uvs {
                if let image = images[key] {
                    image.draw(
                        in: CGRect(
                            x: CGFloat(rect.x),
                            y: CGFloat(rect.y),
                            width: CGFloat(rect.width),
                            height: CGFloat(rect.height)
                        ))
                }
            }

            return UIGraphicsGetImageFromCurrentImageContext()

        #elseif canImport(AppKit)
            let image = NSImage(size: canvasSize)
            image.lockFocus()

            for (key, rect) in uvs {
                if let img = images[key] {
                    let destRect = CGRect(
                        x: CGFloat(rect.x),
                        y: CGFloat(rect.y),
                        width: CGFloat(rect.width),
                        height: CGFloat(rect.height)
                    )
                    img.draw(in: destRect, from: .zero, operation: .sourceOver, fraction: 1.0)
                }
            }

            image.unlockFocus()
            return image

        #else
            return nil
        #endif
    }
}
