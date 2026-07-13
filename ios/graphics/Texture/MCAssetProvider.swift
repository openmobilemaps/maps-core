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

#if canImport(UIKit)
    import UIKit
#elseif canImport(AppKit) && !canImport(UIKit)
    import AppKit
#endif

/// `MCAssetProvider` is a class designed for packing custom icons into a vector layer for use in a mapping application. It implements the `MCTiled2dMapVectorLayerSymbolDelegateInterface` to provide custom assets for vector layer symbols.
///
/// This class allows you to efficiently pack multiple custom icons into a single texture atlas, optimizing performance for rendering vector layers with custom icons.
///
/// ## Usage:
/// 1. Subclass `MCAssetProvider` and implement the `getImageFor` method to provide custom icons for specific feature information.
/// 2. Implement the `getCustomAssets` method to pack the custom icons into texture atlases for use in the vector layer.
///
/// Example:
/// ```swift
/// class CustomAssetProvider: MCAssetProvider {
///     override func getImageFor(for featureInfo: MCVectorLayerFeatureInfo, layerIdentifier: String) -> MCPlatformImage {
///         // Provide a custom icon for the given featureInfo and layerIdentifier.
///     }
/// }
open class MCAssetProvider: MCTiled2dMapVectorLayerSymbolDelegateInterface {
    let scale: CGFloat
    public init() {
        self.scale =
            if Thread.isMainThread {
                MainActor.assumeIsolated {
                    MCDisplayMetrics.nativeScale(for: nil)
                }
            } else {
                DispatchQueue.main.sync {
                    MCDisplayMetrics.nativeScale(for: nil)
                }
            }
    }

    open func getCustomAssets(for featureInfos: [MCVectorLayerFeatureInfo], layerIdentifier: String) -> [MCTiled2dMapVectorAssetInfo] {
        var images: [String: MCPlatformImage] = [:]

        for featureInfo in featureInfos {
            images[featureInfo.identifier] = getImageFor(for: featureInfo, layerIdentifier: layerIdentifier)
        }

        let packerResult = MCRectanglePacker.pack(images.mapValues { MCVec2I(x: Int32($0.size.width * scale), y: Int32($0.size.height * scale)) }, maxPageSize: MCVec2I(x: 4096, y: 4096), spacing: 1)

        var results = [MCTiled2dMapVectorAssetInfo]()

        for page in packerResult {
            let sizes = page.uvs.values.map { CGSize(width: CGFloat($0.x + $0.width), height: CGFloat($0.y + $0.height)) }
            let maxWidth = sizes.map(\.width).max() ?? 0.0
            let maxHeight = sizes.map(\.height).max() ?? 0.0

            #if canImport(UIKit)
                UIGraphicsBeginImageContext(.init(width: maxWidth, height: maxHeight))

                for (key, rect) in page.uvs {
                    if let image = images[key] {
                        image.draw(in: CGRect(x: CGFloat(rect.x), y: CGFloat(rect.y), width: CGFloat(rect.width), height: CGFloat(rect.height)))
                    }
                }

                if let combinedImage = UIGraphicsGetImageFromCurrentImageContext(),
                    let cgImage = combinedImage.cgImage
                {
                    results.append(MCTiled2dMapVectorAssetInfo(featureIdentifiersUv: page.uvs, texture: try? TextureHolder(cgImage)))
                } else {
                    assertionFailure("could not create image atlas")
                }

                UIGraphicsEndImageContext()
            #elseif canImport(AppKit) && !canImport(UIKit)
                let combinedImage = NSImage(size: CGSize(width: maxWidth, height: maxHeight))
                combinedImage.lockFocus()
                for (key, rect) in page.uvs {
                    if let image = images[key] {
                        image.draw(
                            in: CGRect(
                                x: CGFloat(rect.x),
                                y: CGFloat(rect.y),
                                width: CGFloat(rect.width),
                                height: CGFloat(rect.height)
                            ),
                            from: .zero,
                            operation: .sourceOver,
                            fraction: 1.0
                        )
                    }
                }
                combinedImage.unlockFocus()

                if let cgImage = combinedImage.mcCgImage {
                    results.append(MCTiled2dMapVectorAssetInfo(featureIdentifiersUv: page.uvs, texture: try? TextureHolder(cgImage)))
                } else {
                    assertionFailure("could not create image atlas")
                }
            #endif
        }

        return results
    }

    open func getImageFor(for featureInfo: MCVectorLayerFeatureInfo, layerIdentifier: String) -> MCPlatformImage {
        fatalError("implemented by subclass")
    }
}
