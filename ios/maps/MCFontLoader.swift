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
import os
import UIKit

open class MCFontLoader: NSObject, MCFontLoaderInterface {
    // MARK: - Font Atlas Dictionary

    private let loadingQueue = DispatchQueue(label: "MCFontLoader")
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
                os_log("MCFontLoader: unable to load font image for %@", log: OSLog.default, type: .error, font.name)
                return .init(imageData: nil, fontData: nil, status: .ERROR_OTHER)
            }

            guard let data = getFontData(font: font) else {
                os_log("MCFontLoader: unable to load font data for %@", log: OSLog.default, type: .error, font.name)
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
                    let fontInfoJson = jsonResult["info"] as! [String: AnyObject]
                    let commonJson = jsonResult["common"] as! [String: AnyObject]

                    let size = double(dict: fontInfoJson, value: "size")
                    let imageSize = double(dict: commonJson, value: "scaleW")

                    let fontInfo = MCFontWrapper(name: font.name,
                                                 lineHeight: double(dict: commonJson, value: "lineHeight") / size,
                                                 base: double(dict: commonJson, value: "base") / size,
                                                 bitmapSize: MCVec2D(x: imageSize, y: imageSize),
                                                 size: Double(UIScreen.pixelsPerInch) * size)

                    var glyphs: [MCFontGlyph] = []

                    for g in jsonResult["chars"] as! [NSDictionary] {
                        var glyph: [String: AnyObject] = [:]
                        for a in g {
                            glyph[a.key as! String] = a.value as AnyObject
                        }

                        let character = string(dict: glyph, value: "char")

                        var s0 = double(dict: glyph, value: "x")
                        var s1 = s0 + double(dict: glyph, value: "width")
                        var t0 = double(dict: glyph, value: "y")
                        var t1 = t0 + double(dict: glyph, value: "height")

                        s0 = s0 / imageSize
                        s1 = s1 / imageSize
                        t0 = t0 / imageSize
                        t1 = t1 / imageSize

                        let bearing = MCVec2D(x: double(dict: glyph, value: "xoffset") / size, y: -double(dict: glyph, value: "yoffset") / size)

                        let uv = MCQuad2dD(topLeft: MCVec2D(x: s0, y: t1), topRight: MCVec2D(x: s1, y: t1), bottomRight: MCVec2D(x: s1, y: t0), bottomLeft: MCVec2D(x: s0, y: t0))

                        let glyphGlyhph = MCFontGlyph(charCode: character, advance: MCVec2D(x: double(dict: glyph, value: "xadvance") / size, y: 0.0), boundingBoxSize: MCVec2D(x: double(dict: glyph, value: "width") / size, y: double(dict: glyph, value: "height") / size), bearing: bearing, uv: uv)
                        glyphs.append(glyphGlyhph)
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
