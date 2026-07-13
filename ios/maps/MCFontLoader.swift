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

open class MCFontLoader: NSObject, MCFontLoaderInterface, @unchecked Sendable {
    // MARK: - Font Atlas Dictionary

    private let loadingQueue = DispatchQueue(label: "MCFontLoader")
    private var fontAtlasDictionary: [String: TextureHolder] = [:]
    private var fontDataDictionary: [String: MCFontData] = [:]
    private let pixelsPerInch: Double

    // MARK: - Init
    private let bundle: Bundle

    // the bundle to use for searching for fonts
    //
    // `resourcePath` is accepted for backwards-compat with the pre-feature/kmp-2 init
    // signature (e.g. fluid-meteogram's `OpenMobileMapsKmpBridge.createFontLoader`). The
    // parameter is ignored on this branch — fonts are resolved directly inside the provided
    // bundle.
    public convenience init(bundle: Bundle, resourcePath: String?, preload: [String] = []) {
        self.init(bundle: bundle, preload: preload)
    }

    // Single-argument Obj-C-exposed init so Kotlin/Native's swiftPMImport can call
    // `MCFontLoader(bundle: bundle)` without going through an Obj-C bridge class. The Swift
    // `init(bundle:preload:)` below takes a default-valued `preload: [String] = []` which isn't
    // visible to Obj-C, so we need an explicit selector here.
    @objc(initWithBundle:)
    public convenience init(bundle: Bundle) {
        self.init(bundle: bundle, preload: [])
    }

    public init(bundle: Bundle, preload: [String] = []) {
        self.bundle = bundle
        pixelsPerInch =
            if Thread.isMainThread {
                MainActor.assumeIsolated {
                    Double(MCDisplayMetrics.pixelsPerInch(for: nil))
                }
            } else {
                DispatchQueue.main.sync {
                    MainActor.assumeIsolated {
                        Double(MCDisplayMetrics.pixelsPerInch(for: nil))
                    }
                }
            }
        super.init()
        loadingQueue.async {
            let fonts = preload.map { MCFont(name: $0) }
            for font in fonts {
                let _ = self.getFontImage(font: font)
                let _ = self.getFontData(font: font)
            }
        }
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
                if let fontData = Self.parseFontData(data, fontName: font.name) {
                    fontDataDictionary[font.name] = fontData

                    return fontData
                }
            } catch {
                // handle error
            }
        }

        return nil
    }

    public static func parseFontData(_ data: Data, fontName: String) -> MCFontData? {
        guard
            let jsonResult = try? JSONSerialization.jsonObject(with: data, options: .mutableLeaves) as? [String: AnyObject],
            let fontInfoJson = jsonResult["info"] as? [String: AnyObject],
            let commonJson = jsonResult["common"] as? [String: AnyObject],
            let distanceFieldJson = jsonResult["distanceField"] as? [String: AnyObject],
            let chars = jsonResult["chars"] as? [NSDictionary]
        else {
            return nil
        }

        let size = double(dict: fontInfoJson, value: "size")
        let imageSize = double(dict: commonJson, value: "scaleW")
        guard size != 0.0, imageSize != 0.0 else {
            return nil
        }

        let fontInfo = MCFontWrapper(
            name: fontName,
            lineHeight: double(dict: commonJson, value: "lineHeight") / size,
            base: double(dict: commonJson, value: "base") / size,
            bitmapSize: MCVec2D(x: imageSize, y: imageSize),
            size: size,
            distanceRange: double(dict: distanceFieldJson, value: "distanceRange"))

        var glyphs: [MCFontGlyph] = []
        glyphs.reserveCapacity(chars.count)

        for entry in chars {
            var glyph: [String: AnyObject] = [:]
            for attribute in entry {
                guard let key = attribute.key as? String else { continue }
                glyph[key] = attribute.value as AnyObject
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

            let bearing = MCVec2D(
                x: double(dict: glyph, value: "xoffset") / size,
                y: -double(dict: glyph, value: "yoffset") / size
            )

            let uv = MCQuad2dD(
                topLeft: MCVec2D(x: s0, y: t1),
                topRight: MCVec2D(x: s1, y: t1),
                bottomRight: MCVec2D(x: s1, y: t0),
                bottomLeft: MCVec2D(x: s0, y: t0)
            )

            glyphs.append(
                MCFontGlyph(
                    charCode: character,
                    advance: MCVec2D(x: double(dict: glyph, value: "xadvance") / size, y: 0.0),
                    boundingBoxSize: MCVec2D(
                        x: double(dict: glyph, value: "width") / size,
                        y: double(dict: glyph, value: "height") / size
                    ),
                    bearing: bearing,
                    uv: uv
                )
            )
        }

        return MCFontData(info: fontInfo, glyphs: glyphs)
    }

    private static func double(dict: [String: AnyObject], value: String) -> Double {
        (dict[value] as? NSNumber)?.doubleValue ?? 0.0
    }

    private static func string(dict: [String: AnyObject], value: String) -> String {
        (dict[value] as? String) ?? ""
    }

    private func getFontImage(font: MCFont) -> TextureHolder? {
        if let fontData = fontAtlasDictionary[font.name] {
            return fontData
        }

        let textureHolder: TextureHolder?
        if let path = bundle.path(forResource: font.name, ofType: "png"),
            let data = try? Data(contentsOf: URL(fileURLWithPath: path), options: .mappedIfSafe)
        {
            textureHolder = try? TextureHolder(data)
        } else {
            textureHolder = nil
        }
        guard let textureHolder else { return nil }

        fontAtlasDictionary[font.name] = textureHolder

        return textureHolder
    }
}
