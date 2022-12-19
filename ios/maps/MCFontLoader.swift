/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import MapCoreSharedModule
import UIKit

open class MCFontLoader: NSObject, MCFontLoaderInterface {
    // MARK: - Font Atlas Dictionary

    private let loadingQueue = DispatchQueue(label: "MCFontLoader", qos: .userInitiated)
    private var fontAtlasDictionary: [String: TextureHolder] = [:]
    private var fontDataDictionary: [String: MCFontData] = [:]

    // MARK: - Init

    private let bundle: Bundle

    // the bundle to use for searching for fonts
    public init(bundle: Bundle) {
        self.bundle = bundle
        super.init()
    }

    // MARK: - Loader


    public func load(_ font: MCFont) -> MCFontLoaderResult {
        loadingQueue.sync {
            guard let image = getFontImage(font: font) else {
                return .init(imageData: nil, fontData: nil, status: .ERROR_OTHER)
            }

            guard let data = getFontData(font: font) else {
                return .init(imageData: nil, fontData: nil, status: .ERROR_OTHER)
            }

            return .init(imageData: image, fontData: data, status: .OK)
        }
    }


    private func getFontData(font: MCFont) -> MCFontData? {
        if let fontData = fontDataDictionary[font.name] {
            return fontData
        }
        if let path = bundle.path(forResource: font.name, ofType: "json") {
            do {
                let data = try Data(contentsOf: URL(fileURLWithPath: path), options: .mappedIfSafe)
                let jsonResult = try JSONSerialization.jsonObject(with: data, options: .mutableLeaves)
                if let jsonResult = jsonResult as? [String: AnyObject] {
                    let fontInfo = MCFontWrapper(name: string(dict: jsonResult, value: "name"), ascender: double(dict: jsonResult, value: "ascender"), descender: double(dict: jsonResult, value: "descender"), spaceAdvance: double(dict: jsonResult, value: "space_advance"), bitmapSize: MCVec2D(x: double(dict: jsonResult, value: "bitmap_width"), y: double(dict: jsonResult, value: "bitmap_height")), size: Double(UIScreen.main.nativeScale) * double(dict: jsonResult, value: "size"))

                    var glyphs: [MCFontGlyph] = []
                    for glyph in jsonResult["glyph_data"] as? [String: AnyObject] ?? [:] {
                        let character = glyph.key

                        if let d = glyph.value as? [String: AnyObject] {
                            let s0 = double(dict: d, value: "s0")
                            let s1 = double(dict: d, value: "s1")
                            let t0 = 1.0 - double(dict: d, value: "t0")
                            let t1 = 1.0 - double(dict: d, value: "t1")
                            let bearing = MCVec2D(x: double(dict: d, value: "bearing_x"), y: double(dict: d, value: "bearing_y"))

                            let uv = MCQuad2dD(topLeft: MCVec2D(x: s0, y: t1), topRight: MCVec2D(x: s1, y: t1), bottomRight: MCVec2D(x: s1, y: t0), bottomLeft: MCVec2D(x: s0, y: t0))

                            let glyph = MCFontGlyph(charCode: character, advance: MCVec2D(x: double(dict: d, value: "advance_x"), y: double(dict: d, value: "advance_y")), boundingBoxSize: MCVec2D(x: double(dict: d, value: "bbox_width"), y: double(dict: d, value: "bbox_height")), bearing: bearing, uv: uv)
                            glyphs.append(glyph)
                        }
                    }

                    let fontData = MCFontData(info: fontInfo, glyphs: glyphs)

                    fontDataDictionary[font.name] = fontData

                    return fontData
                }
            } catch {
                // handle error
            }
        }

        return nil
    }

    private func double(dict: [String: AnyObject], value: String) -> Double {
        (dict[value] as? NSNumber)?.doubleValue ?? 0.0
    }

    private func string(dict: [String: AnyObject], value: String) -> String {
        (dict[value] as? String) ?? ""
    }

    private func getFontImage(font: MCFont) -> TextureHolder? {
        if let fontData = fontAtlasDictionary[font.name] {
            return fontData
        }

        let image = UIImage(named: font.name, in: bundle, compatibleWith: nil)

        guard let cgImage = image?.cgImage,
                let textureHolder = try? TextureHolder(cgImage) else {
            return nil
        }

        fontAtlasDictionary[font.name] = textureHolder

        return textureHolder
    }
}
