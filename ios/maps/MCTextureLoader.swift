/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import DjinniSupport
import Foundation
import MapCoreSharedModule
import OSLog

#if canImport(UIKit)
    import UIKit
#elseif canImport(AppKit) && !canImport(UIKit)
    import AppKit
#endif

private let logger = Logger(subsystem: "maps-core", category: "MCTextureLoader")

@objc open class MCTextureLoader: NSObject, MCLoaderInterface, @unchecked Sendable {
    public let session: URLSession

    public var isRasterDebugModeEnabled: Bool

    public var taskQueue = DispatchQueue(label: "MCTextureLoader.tasks")
    public var tasks: [String: URLSessionTask] = [:]

    public let urlCache = URLCache(memoryCapacity: 100 * 1024 * 1024, diskCapacity: 500 * 1024 * 1024, diskPath: "ch.openmobilemaps.urlcache")

    public convenience override init() {
        self.init(urlSession: nil)
    }

    public init(urlSession: URLSession?) {
        if let urlSession {
            session = urlSession
        } else {
            let sessionConfig = URLSessionConfiguration.default
            sessionConfig.urlCache = urlCache
            sessionConfig.networkServiceType = .responsiveData
            session = .init(configuration: sessionConfig)
        }

        isRasterDebugModeEnabled = UserDefaults.standard.bool(forKey: "io.openmobilemaps.debug.rastertiles.enabled")
    }

    open func loadTexture(_ url: String, etag: String?) -> MCTextureLoaderResult {
        let semaphore = DispatchSemaphore(value: 0)
        var result: MCTextureLoaderResult? = nil
        loadTextureAsync(url, etag: etag)
            .then { future in
                result = future.get()
                semaphore.signal()
                return nil
            }

        if semaphore.wait(timeout: .now() + 30.0) == .timedOut {
            return MCTextureLoaderResult(data: nil, etag: nil, status: .ERROR_TIMEOUT, errorCode: "SEMTIM")
        }

        if let result {
            return result
        }

        return MCTextureLoaderResult(data: nil, etag: nil, status: .ERROR_OTHER, errorCode: "NRES")
    }

    open func loadTextureAsync(_ url: String, etag: String?) -> DJFuture<MCTextureLoaderResult> {
        let urlString = url

        let promise = DJPromise<MCTextureLoaderResult>()

        guard let url = URL(string: urlString) else {
            assertionFailure("invalid url: \(urlString)")
            promise.setValue(.init(data: nil, etag: nil, status: .ERROR_NETWORK, errorCode: "IURL"))
            return promise.getFuture()
        }

        var urlRequest = URLRequest(url: url)

        modifyUrlRequest(request: &urlRequest)

        var wasCached = false
        if isRasterDebugModeEnabled,
            session.configuration.urlCache?.cachedResponse(for: urlRequest) != nil
        {
            wasCached = true
        }

        let canModifyTextureData = canModifyTextureData(for: urlString)

        var task = session.dataTask(with: urlRequest) { [weak self, wasCached, canModifyTextureData] data, response_, error_ in
            guard let self else { return }

            self.taskQueue.sync {
                _ = self.tasks.removeValue(forKey: urlString) == nil
            }

            let result: Data? = data
            let response: HTTPURLResponse? = response_ as? HTTPURLResponse
            let error: NSError? = error_ as NSError?

            if error?.domain == NSURLErrorDomain, error?.code == NSURLErrorTimedOut {
                if #available(iOS 14.0, *) {
                    logger.debug("Failed to load \(url, privacy: .public): Timeout")
                }
                promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_TIMEOUT, errorCode: (error?.code).stringOrNil))
                return
            }

            if error?.domain == NSURLErrorDomain, error?.code == NSURLErrorCancelled {
                promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_OTHER, errorCode: "Cancelled"))
                return
            } else if response?.statusCode == 404 {
                if #available(iOS 14.0, *) {
                    logger.debug("Failed to load \(url, privacy: .public): 404, \(data.map { String(data: $0, encoding: .utf8)?.prefix(1024) ?? "?" } ?? "?")")
                }
                promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_404, errorCode: (response?.statusCode).stringOrNil))
                return
            } else if response?.statusCode == 400 {
                if #available(iOS 14.0, *) {
                    logger.debug("Failed to load \(url, privacy: .public): 400, \(data.map { String(data: $0, encoding: .utf8)?.prefix(1024) ?? "?" } ?? "?")")
                }
                promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_400, errorCode: (response?.statusCode).stringOrNil))
                return
            } else if response?.statusCode == 204 {
                promise.setValue(.init(data: nil, etag: response?.etag, status: .OK, errorCode: nil))
                return
            } else if response?.statusCode != 200 {
                if #available(iOS 14.0, *) {
                    logger.debug("Failed to load \(url, privacy: .public): \(response?.statusCode ?? 0, privacy: .public), \(data.map { String(data: $0, encoding: .utf8)?.prefix(1024) ?? "?" } ?? "?")")
                }
                promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_NETWORK, errorCode: (response?.statusCode).stringOrNil))
                return
            }

            guard let data = result else {
                promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_OTHER, errorCode: (response?.statusCode).stringOrNil))
                return
            }

            do {
                #if canImport(UIKit)
                    if self.isRasterDebugModeEnabled,
                        canModifyTextureData,
                        let uiImage = UIImage(data: data)
                    {
                        let renderer = UIGraphicsImageRenderer(size: uiImage.size)
                        let img = renderer.image { ctx in
                            self.applyDebugWatermark(url: urlString, image: uiImage, wasCached: wasCached, ctx: ctx)
                        }
                        if let cgImage = img.cgImage,
                            let textureHolder = try? TextureHolder(cgImage)
                        {
                            promise.setValue(.init(data: textureHolder, etag: response?.etag, status: .OK, errorCode: nil))
                            return
                        }
                    }
                #endif

                let textureHolder = try TextureHolder(data)
                promise.setValue(.init(data: textureHolder, etag: response?.etag, status: .OK, errorCode: nil))
                return
            } catch TextureHolderError.emptyData {
                promise.setValue(.init(data: nil, etag: response?.etag, status: .OK, errorCode: nil))
                return
            } catch {
                #if canImport(UIKit)
                    // If metal can not load this image, try to redraw it in a UIKit context.
                    guard canModifyTextureData else {
                        promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_OTHER, errorCode: "MNL"))
                        return
                    }

                    guard let uiImage = UIImage(data: data) else {
                        promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_OTHER, errorCode: "MNL"))
                        return
                    }

                    let renderer = UIGraphicsImageRenderer(size: uiImage.size)
                    let img = renderer.image { ctx in
                        if self.isRasterDebugModeEnabled {
                            self.applyDebugWatermark(url: urlString, image: uiImage, wasCached: wasCached, ctx: ctx)
                        } else {
                            uiImage.draw(in: .init(origin: .init(), size: uiImage.size))
                        }
                    }

                    guard let cgImage = img.cgImage,
                        let textureHolder = try? TextureHolder(cgImage)
                    else {
                        promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_OTHER, errorCode: "UINL"))
                        return
                    }

                    promise.setValue(.init(data: textureHolder, etag: response?.etag, status: .OK, errorCode: nil))
                #elseif canImport(AppKit) && !canImport(UIKit)
                    // If metal can not load this image, try to redraw it in a AppKit context.
                    guard canModifyTextureData else {
                        promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_OTHER, errorCode: "MNL"))
                        return
                    }

                    guard let nsImage = NSImage(data: data) else {
                        promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_OTHER, errorCode: "MNL"))
                        return
                    }

                    let img = NSImage(size: nsImage.size, flipped: false) { rect in
                        nsImage.draw(in: .init(origin: .zero, size: nsImage.size))
                        return true
                    }

                    guard let cgImage = img.cgImage(forProposedRect: nil, context: nil, hints: nil),
                        let textureHolder = try? TextureHolder(cgImage)
                    else {
                        promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_OTHER, errorCode: "UINL"))
                        return
                    }

                    promise.setValue(.init(data: textureHolder, etag: response?.etag, status: .OK, errorCode: nil))
                #else
                    promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_OTHER, errorCode: "MNL"))
                #endif
                return
            }
        }

        taskQueue.sync {
            tasks[urlString] = task
        }

        modifyDataTask(task: &task)

        task.resume()
        return promise.getFuture()
    }

    open func loadData(_ url: String, etag: String?) -> MCDataLoaderResult {
        let semaphore = DispatchSemaphore(value: 0)
        var result: MCDataLoaderResult? = nil
        loadDataAsync(url, etag: etag)
            .then { future in
                result = future.get()
                semaphore.signal()
                return nil
            }

        if semaphore.wait(timeout: .now() + 30.0) == .timedOut {
            return MCDataLoaderResult(data: nil, etag: nil, status: .ERROR_TIMEOUT, errorCode: "SEMTIM")
        }

        if let result {
            return result
        }

        return MCDataLoaderResult(data: nil, etag: nil, status: .ERROR_OTHER, errorCode: "NRES")
    }

    open func loadDataAsync(_ url: String, etag: String?) -> DJFuture<MCDataLoaderResult> {
        let urlString = url

        let promise = DJPromise<MCDataLoaderResult>()

        guard let url = URL(string: urlString) else {
            assertionFailure("invalid url: \(urlString)")
            promise.setValue(.init(data: nil, etag: nil, status: .ERROR_NETWORK, errorCode: "IURL"))
            return promise.getFuture()
        }

        var urlRequest = URLRequest(url: url)

        modifyUrlRequest(request: &urlRequest)

        var task = session.dataTask(with: urlRequest) { [weak self] data, response_, error_ in
            guard let self else { return }

            self.taskQueue.sync {
                _ = self.tasks.removeValue(forKey: urlString) == nil
            }

            let result: Data? = data
            let response: HTTPURLResponse? = response_ as? HTTPURLResponse
            let error: NSError? = error_ as NSError?

            if error?.domain == NSURLErrorDomain, error?.code == NSURLErrorTimedOut {
                if #available(iOS 14.0, *) {
                    logger.debug("Failed to load \(url, privacy: .public): Timeout")
                }
                promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_TIMEOUT, errorCode: (error?.code).stringOrNil))
                return
            }

            if error?.domain == NSURLErrorDomain, error?.code == NSURLErrorCancelled {
                promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_OTHER, errorCode: "Cancelled"))
                return
            } else if response?.statusCode == 404 {
                if #available(iOS 14.0, *) {
                    logger.debug("Failed to load \(url, privacy: .public): 404, \(data.map { String(data: $0, encoding: .utf8)?.prefix(1024) ?? "?" } ?? "?")")
                }
                promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_404, errorCode: (response?.statusCode).stringOrNil))
                return
            } else if response?.statusCode == 400 {
                if #available(iOS 14.0, *) {
                    logger.debug("Failed to load \(url, privacy: .public): 400, \(data.map { String(data: $0, encoding: .utf8)?.prefix(1024) ?? "?" } ?? "?")")
                }
                promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_400, errorCode: (response?.statusCode).stringOrNil))
                return
            } else if response?.statusCode == 204 {
                promise.setValue(.init(data: nil, etag: response?.etag, status: .OK, errorCode: nil))
                return
            } else if response?.statusCode != 200 {
                if #available(iOS 14.0, *) {
                    logger.debug("Failed to load \(url, privacy: .public): \(response?.statusCode ?? 0, privacy: .public), \(data.map { String(data: $0, encoding: .utf8)?.prefix(1024) ?? "?" } ?? "?")")
                }
                promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_NETWORK, errorCode: (response?.statusCode).stringOrNil))
                return
            }

            guard let data = result else {
                promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_OTHER, errorCode: (response?.statusCode).stringOrNil))
                return
            }

            promise.setValue(.init(data: data, etag: response?.etag, status: .OK, errorCode: nil))
        }

        taskQueue.sync {
            tasks[urlString] = task
        }

        modifyDataTask(task: &task)

        task.resume()
        return promise.getFuture()
    }

    public func cancel(_ url: String) {
        self.taskQueue.sync {
            if let task = self.tasks[url] {
                task.cancel()
            }
        }
    }

    open func modifyUrlRequest(request _: inout URLRequest) {
    }

    open func modifyDataTask(task _: inout URLSessionDataTask) {
    }

    open func canModifyTextureData(for url: String) -> Bool {
        !Self.isLikelyDEMTextureURL(url)
    }

    private static func isLikelyDEMTextureURL(_ urlString: String) -> Bool {
        let lowercasedURL = urlString.lowercased()
        let demMarkers = [
            "-dem",
            "terrain-rgb",
            "terrarium",
            "mapterhorn",
        ]
        if demMarkers.contains(where: lowercasedURL.contains) {
            return true
        }

        guard let components = URLComponents(string: urlString) else {
            return false
        }

        let pathComponents = components.path
            .lowercased()
            .split(separator: "/")
            .map(String.init)

        return pathComponents.contains("dem") || pathComponents.contains("elevation")
    }

    #if canImport(UIKit)
        func applyDebugWatermark(url: String, image: UIImage, wasCached: Bool, ctx: UIGraphicsRendererContext) {
            let size = image.size

            image.draw(in: .init(origin: .init(), size: size))

            guard isRasterDebugModeEnabled else { return }

            ctx.cgContext.setStrokeColor(UIColor.black.cgColor)
            ctx.cgContext.setLineWidth(2.0)
            ctx.cgContext.stroke(.init(origin: .init(), size: size).inset(by: .init()))

            let label = Self.rasterDebugOverlayLabel(for: url)
            let font = UIFont.monospacedSystemFont(ofSize: max(10.0, min(16.0, size.width / 22.0)), weight: .bold)
            let attrs: [NSAttributedString.Key: Any] = [
                .font: font,
                .foregroundColor: UIColor.white,
            ]
            let labelSize = (label as NSString).size(withAttributes: attrs)
            let padding: CGFloat = max(4.0, min(size.width, size.height) * 0.025)
            let horizontalPadding = padding * 2.0
            let labelBackgroundColor = wasCached ? UIColor.darkGray.withAlphaComponent(0.85) : UIColor(red: 1, green: 0, blue: 0.55, alpha: 1)
            let labelRect = CGRect(
                x: padding,
                y: padding,
                width: min(size.width - padding * 2, labelSize.width + horizontalPadding * 2),
                height: labelSize.height + padding
            )
            ctx.cgContext.setFillColor(labelBackgroundColor.cgColor)
            ctx.cgContext.fill(labelRect)

            label.draw(
                with: labelRect.insetBy(dx: horizontalPadding, dy: padding / 2),
                options: .usesLineFragmentOrigin,
                attributes: attrs,
                context: nil
            )
        }
    #endif

    private static func rasterDebugOverlayLabel(for urlString: String) -> String {
        if let label = rasterDebugOverlayLabelFromKeyValuePairs(in: urlString) {
            return label
        }

        guard let components = URLComponents(string: urlString) else {
            return "tile"
        }

        if let queryItems = components.queryItems {
            if let x = queryItems.firstValue(named: "x"),
                let y = queryItems.firstValue(named: "y"),
                let z = queryItems.firstValue(named: "z")
            {
                return "z=\(z) x=\(x) y=\(y)"
            }
        }

        let pathComponents = components.path
            .split(separator: "/")
            .map(String.init)

        if pathComponents.count >= 3,
            let z = rasterDebugTileCoordinate(from: pathComponents[pathComponents.count - 3]),
            let x = rasterDebugTileCoordinate(from: pathComponents[pathComponents.count - 2]),
            let y = rasterDebugTileCoordinate(from: pathComponents[pathComponents.count - 1])
        {
            return "z=\(z) x=\(x) y=\(y)"
        }

        return "tile"
    }

    private static func rasterDebugOverlayLabelFromKeyValuePairs(in urlString: String) -> String? {
        let values =
            urlString
            .split(whereSeparator: { $0 == "?" || $0 == "&" })
            .reduce(into: [String: String]()) { result, part in
                guard let separatorIndex = part.firstIndex(of: "=") else { return }
                let key = part[..<separatorIndex].lowercased()
                let valueStartIndex = part.index(after: separatorIndex)
                guard key == "x" || key == "y" || key == "z" else { return }
                result[String(key)] = String(part[valueStartIndex...])
            }

        guard let x = values["x"], let y = values["y"], let z = values["z"] else {
            return nil
        }

        return "z=\(z) x=\(x) y=\(y)"
    }

    private static func rasterDebugTileCoordinate(from component: String) -> Int? {
        let stem = (component as NSString).deletingPathExtension
        let digits = stem.prefix { character in
            character.unicodeScalars.allSatisfy { CharacterSet.decimalDigits.contains($0) }
        }
        guard !digits.isEmpty else { return nil }
        return Int(String(digits))
    }
}

private extension Array where Element == URLQueryItem {
    func firstValue(named name: String) -> String? {
        first { $0.name.caseInsensitiveCompare(name) == .orderedSame }?.value
    }
}

extension HTTPURLResponse {
    public var etag: String? {
        let etag: String?
        if #available(iOS 13.0, *) {
            etag = value(forHTTPHeaderField: "ETag")
        } else {
            etag = allHeaderFields["ETag"] as? String
        }
        return etag
    }
}

public extension Int? {
    var stringOrNil: String {
        switch self {
            case .none:
                return ""
            case let .some(wrapped):
                return "\(wrapped)"
        }
    }
}
