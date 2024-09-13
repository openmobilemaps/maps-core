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
import MapCoreSharedModule
import OSLog
import UIKit

@available(iOS 14.0, *)
private let logger = Logger(subsystem: "maps-core", category: "MCTextureLoader")

open class MCTextureLoader: MCLoaderInterface {
    private let session: URLSession

    public var isRasterDebugModeEnabled: Bool

    var taskQueue = DispatchQueue(label: "MCTextureLoader.tasks")
    var tasks: [String: URLSessionTask] = [:]

    public let urlCache = URLCache(memoryCapacity: 100 * 1024 * 1024, diskCapacity: 500 * 1024 * 1024, diskPath: "ch.openmobilemaps.urlcache")

    public init(urlSession: URLSession? = nil) {
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
        loadTextureAsnyc(url, etag: etag).then { future in
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

    open func loadTextureAsnyc(_ url: String, etag: String?) -> DJFuture<MCTextureLoaderResult> {
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
           session.configuration.urlCache?.cachedResponse(for: urlRequest) != nil {
            wasCached = true
        }

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
                // Do nothing, since the result is dropped anyway (setting a LoaderStatus will cause the SharedLib to do further computing)
                return
            }

            if response?.statusCode == 404 {
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
                if self.isRasterDebugModeEnabled,
                   let uiImage = UIImage(data: data) {
                    let renderer = UIGraphicsImageRenderer(size: uiImage.size)
                    let img = renderer.image { ctx in
                        self.applyDebugWatermark(url: urlString, byteCount: data.count, image: uiImage, wasCached: wasCached, ctx: ctx)
                    }
                    if let cgImage = img.cgImage,
                       let textureHolder = try? TextureHolder(cgImage) {
                        promise.setValue(.init(data: textureHolder, etag: response?.etag, status: .OK, errorCode: nil))
                        return
                    }
                }

                let textureHolder = try TextureHolder(data)
                promise.setValue(.init(data: textureHolder, etag: response?.etag, status: .OK, errorCode: nil))
                return
            } catch TextureHolderError.emptyData {
                promise.setValue(.init(data: nil, etag: response?.etag, status: .OK, errorCode: nil))
                return
            } catch {
                // If metal can not load this image
                // try workaround to first load it into UIImage context
                guard let uiImage = UIImage(data: data) else {
                    promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_OTHER, errorCode: "MNL"))
                    return
                }

                let renderer = UIGraphicsImageRenderer(size: uiImage.size)
                let img = renderer.image { ctx in
                    if self.isRasterDebugModeEnabled {
                        self.applyDebugWatermark(url: urlString, byteCount: data.count, image: uiImage, wasCached: wasCached, ctx: ctx)
                    } else {
                        uiImage.draw(in: .init(origin: .init(), size: uiImage.size))
                    }
                }

                guard let cgImage = img.cgImage,
                      let textureHolder = try? TextureHolder(cgImage) else {
                    promise.setValue(.init(data: nil, etag: response?.etag, status: .ERROR_OTHER, errorCode: "UINL"))
                    return
                }

                promise.setValue(.init(data: textureHolder, etag: response?.etag, status: .OK, errorCode: nil))
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
        loadDataAsync(url, etag: etag).then { future in
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
                // Do nothing, since the result is dropped anyway (setting a LoaderStatus will cause the SharedLib to do further computing)
                return
            }

            if response?.statusCode == 404 {
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

    func applyDebugWatermark(url: String, byteCount: Int, image: UIImage, wasCached: Bool, ctx: UIGraphicsRendererContext) {
        let size = image.size

        image.draw(in: .init(origin: .init(), size: size))

        guard isRasterDebugModeEnabled else { return }

        ctx.cgContext.setFillColor(gray: 0.1, alpha: 0.2)
        ctx.fill(.init(origin: .init(), size: size), blendMode: .normal)

        ctx.cgContext.setStrokeColor(UIColor.black.cgColor)
        ctx.cgContext.setLineWidth(5.0)
        ctx.cgContext.stroke(.init(origin: .init(), size: size).inset(by: .init()))

        let paragraphStyle = NSMutableParagraphStyle()
        paragraphStyle.alignment = .center

        let attrs: [NSAttributedString.Key: Any] = [NSAttributedString.Key.paragraphStyle: paragraphStyle,
                                                    NSAttributedString.Key.backgroundColor: wasCached ? UIColor.lightGray.cgColor : UIColor.white.cgColor]

        let byteCountString = ByteCountFormatter().string(fromByteCount: Int64(byteCount))
        let loadedString = wasCached ? "Loaded from Cache" : "Loaded from www"

        let string = url + "\n" + loadedString + " at: " + ISO8601DateFormatter().string(from: .init()) + "\nSize: " + byteCountString
        string.draw(with: .init(origin: .init(x: 0, y: 25), size: size).inset(by: .init(top: 5, left: 5, bottom: 5, right: 5)), options: .usesLineFragmentOrigin, attributes: attrs, context: nil)
    }
}

extension HTTPURLResponse {
    var etag: String? {
        let etag: String?
        if #available(iOS 13.0, *) {
            etag = value(forHTTPHeaderField: "ETag")
        } else {
            etag = allHeaderFields["ETag"] as? String
        }
        return etag
    }
}

private extension Int? {
    var stringOrNil: String {
        switch self {
            case .none:
                return ""
            case let .some(wrapped):
                return "\(wrapped)"
        }
    }
}
