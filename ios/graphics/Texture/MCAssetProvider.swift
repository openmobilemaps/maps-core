/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import UIKit

/**
 `MCAssetProvider` is a class designed for packing custom icons into a vector layer for use in a mapping application. It implements the `MCTiled2dMapVectorLayerSymbolDelegateInterface` to provide custom assets for vector layer symbols.

 This class allows you to efficiently pack multiple custom icons into a single texture atlas, optimizing performance for rendering vector layers with custom icons.

 ## Usage:
 1. Subclass `MCAssetProvider` and implement the `getImageFor` method to provide custom icons for specific feature information.
 2. Implement the `getCustomAssets` method to pack the custom icons into texture atlases for use in the vector layer.

 Example:
 ```swift
 class CustomAssetProvider: MCAssetProvider {
     override func getImageFor(for featureInfo: MCVectorLayerFeatureInfo, layerIdentifier: String) -> UIImage {
         // Provide a custom icon for the given featureInfo and layerIdentifier.
     }
 }
 */
open class MCAssetProvider: MCTiled2dMapVectorLayerSymbolDelegateInterface {
    public init() {
    }

    open func getCustomAssets(for featureInfos: [MCVectorLayerFeatureInfo], layerIdentifier: String) -> [MCTiled2dMapVectorAssetInfo] {
        var images: [String: UIImage] = [:]

        for featureInfo in featureInfos {
            images[featureInfo.identifier] = getImageFor(for: featureInfo, layerIdentifier: layerIdentifier)
        }
        let scale = UIScreen.main.nativeScale
        let packerResult = MCRectanglePacker.pack(images.mapValues { MCVec2I(x: Int32($0.size.width * scale), y: Int32($0.size.height * scale)) }, maxPageSize: MCVec2I(x: 4096, y: 4096))

        var results = [MCTiled2dMapVectorAssetInfo]()

        for page in packerResult {
            let sizes = page.uvs.values.map { CGSize(width: CGFloat($0.x + $0.width), height: CGFloat($0.y + $0.height)) }
            let maxWidth = sizes.map(\.width).max() ?? 0.0
            let maxHeight = sizes.map(\.height).max() ?? 0.0

            UIGraphicsBeginImageContext(.init(width: maxWidth, height: maxHeight))

            for (key, rect) in page.uvs {
                if let image = images[key] {
                    image.draw(in: CGRect(x: CGFloat(rect.x), y: CGFloat(rect.y), width: CGFloat(rect.width), height: CGFloat(rect.height)))
                }
            }

            if let combinedImage = UIGraphicsGetImageFromCurrentImageContext() {
                results.append(MCTiled2dMapVectorAssetInfo(featureIdentifiersUv: page.uvs, texture: try? TextureHolder(combinedImage.cgImage!)))
            } else {
                assertionFailure("could not create image atlas")
            }

            UIGraphicsEndImageContext()
        }

        return results
    }

    open func getImageFor(for featureInfo: MCVectorLayerFeatureInfo, layerIdentifier: String) -> UIImage {
        fatalError("implemented by subclass")
    }
}
