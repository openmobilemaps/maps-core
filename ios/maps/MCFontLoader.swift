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

    private var fontAtlasDictionary: [String: UIImage] = [:]
    private var fontDataDictionary: [String: MCFontData] = [:]

    // MARK: - Init

    public override init() {
        super.init()
        setup()
    }

    // MARK: - Loader

    public func addFont(image: UIImage, fontData: MCFontData, for font: MCFont) {
        fontAtlasDictionary[font.name] = image
        fontDataDictionary[font.name] = fontData
    }

    public func load(_ font: MCFont) -> MCFontLoaderResult {
        guard let image = fontAtlasDictionary[font.name] else {
            return .init(imageData: nil, fontData: nil, status: .ERROR_OTHER)
        }

        guard let cgImage = image.cgImage, let textureHolder = try? TextureHolder(cgImage) else {
            return .init(imageData: nil, fontData: nil, status: .ERROR_OTHER)
        }

        guard let data = fontDataDictionary[font.name] else {
            return .init(imageData: nil, fontData: nil, status: .ERROR_OTHER)
        }

        return .init(imageData: textureHolder, fontData: data, status: .OK)
    }

    // MARK: - Setup

    private func setup() {
        let font = MCFont(name: "FrutigerLTStd-Roman")
        let fontData = getFontData(font: font)
        let fontImage = getFontImage(font: font)

        addFont(image: fontImage!, fontData: fontData!, for: font)

        let font2 = MCFont(name: "Frutiger Neue LT Condensed Bold")
        let font2Data = getFontData(font: font2)
        let font2Image = getFontImage(font: font2)

        addFont(image: font2Image!, fontData: font2Data!, for: font2)
    }

    private func getFontData(font: MCFont) -> MCFontData? {
        if let path = Bundle.module.path(forResource: font.name, ofType: "json") {
            do {
                let data = try Data(contentsOf: URL(fileURLWithPath: path), options: .mappedIfSafe)
                let jsonResult = try JSONSerialization.jsonObject(with: data, options: .mutableLeaves)
                if let jsonResult = jsonResult as? [String: AnyObject] {
                    let fontInfo = MCFontInfo(name: string(dict: jsonResult, value: "name"), ascender: double(dict: jsonResult, value: "ascender"), descender: double(dict: jsonResult, value: "descender"), spaceAdvance: double(dict: jsonResult, value: "space_advance"), bitmapSize: MCVec2D(x: double(dict: jsonResult, value: "bitmap_width"), y: double(dict: jsonResult, value: "bitmap_height")), size: Double(UIScreen.main.nativeScale) * double(dict: jsonResult, value: "size"))

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

                    return MCFontData(info: fontInfo, glyphs: glyphs)
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

    private func getFontImage(font: MCFont) -> UIImage? {
         UIImage(named: font.name, in: Bundle.module, compatibleWith: nil)
    }
}
