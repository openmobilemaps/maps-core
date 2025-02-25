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

public protocol CancellableTask {
    func cancel()
}

extension URLSessionTask: CancellableTask {}

open class MCTextureLoader: MCLoaderInterface, @unchecked Sendable {
    public let session: URLSession

    public var isRasterDebugModeEnabled: Bool

    public var taskQueue = DispatchQueue(label: "MCTextureLoader.tasks")
    public var tasks: [String: CancellableTask] = [:]

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

    public struct LoaderResult {
        let data: Data?

        let statusCode: Int
        let etag: String?
        let wasCached: Bool?

        public init(data: Data?, statusCode: Int, etag: String?, wasCached: Bool?) {
            self.data = data
            self.statusCode = statusCode
            self.etag = etag
            self.wasCached = wasCached
        }

        public enum LoaderError: Error {
            case invalidResponse
            case invalidURL
            case timeout
            case cancelled
            case other(Error)
        }
    }

    open func load(url urlString: String, setWasCachedFlag: Bool = false, completion: @escaping (Result<LoaderResult, LoaderResult.LoaderError>) -> Void) -> CancellableTask? {

        guard let url = URL(string: urlString) else {
            completion(.failure(LoaderResult.LoaderError.invalidURL))
            return nil
        }

        var urlRequest = URLRequest(url: url)

        modifyUrlRequest(request: &urlRequest)

        var wasCached: Bool? = nil
        if setWasCachedFlag {
            if session.configuration.urlCache?.cachedResponse(for: urlRequest) != nil {
                wasCached = true
            } else {
                wasCached = false
            }
        }

        var task = session.dataTask(with: urlRequest) { data, response, error in
            if let error {
                if (error as NSError).domain == NSURLErrorDomain, (error as NSError).code == NSURLErrorTimedOut {
                    completion(.failure(LoaderResult.LoaderError.timeout))
                } else if (error as NSError).domain == NSURLErrorDomain, (error as NSError).code == NSURLErrorCancelled {
                    completion(.failure(LoaderResult.LoaderError.cancelled))
                } else {

                }
                completion(.failure(.other(error)))
                return
            }

            guard let response = response as? HTTPURLResponse else {
                completion(.failure(LoaderResult.LoaderError.invalidResponse))
                return
            }

            let result = LoaderResult(data: data, statusCode: response.statusCode, etag: response.etag, wasCached: wasCached)
            completion(.success(result))
        }

        modifyDataTask(task: &task)

        task.resume()

        return task

    }

    open func loadTexture(_ url: String, etag: String?) -> MCTextureLoaderResult {
        let semaphore = DispatchSemaphore(value: 0)
        var result: MCTextureLoaderResult? = nil
        let uuid = UUID()
        loadTextureAsync(url, etag: etag).then { future in
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

        let task = load(url: url, setWasCachedFlag: self.isRasterDebugModeEnabled) { [weak self] result in

            guard let self else { return }

            self.taskQueue.sync {
                _ = self.tasks.removeValue(forKey: urlString) == nil
            }

            switch result {
            case .failure(.timeout):
                if #available(iOS 14.0, *) {
                    logger.debug("Failed to load \(url, privacy: .public): Timeout")
                }
                promise.setValue(.init(data: nil, etag: nil, status: .ERROR_TIMEOUT, errorCode: nil))
            case .failure(.cancelled):
                // Do nothing, since the result is dropped anyway (setting a LoaderStatus will cause the SharedLib to do further computing)
                break
            case .failure(_):
                promise.setValue(.init(data: nil, etag: nil, status: .ERROR_OTHER, errorCode: "UNK"))

            case .success(let result):
                Task {
                    if result.statusCode == 404 {
                        if #available(iOS 14.0, *) {
                            logger.debug("Failed to load \(url, privacy: .public): 404, \(result.data.map { String(data: $0, encoding: .utf8)?.prefix(1024) ?? "?" } ?? "?")")
                        }
                        promise.setValue(.init(data: nil, etag: result.etag, status: .ERROR_404, errorCode: "\(result.statusCode)"))
                    } else if result.statusCode == 400 {
                        if #available(iOS 14.0, *) {
                            logger.debug("Failed to load \(url, privacy: .public): 400, \(result.data.map { String(data: $0, encoding: .utf8)?.prefix(1024) ?? "?" } ?? "?")")
                        }
                        promise.setValue(.init(data: nil, etag: result.etag, status: .ERROR_400, errorCode: "\(result.statusCode)"))
                    } else if result.statusCode == 204 {
                        promise.setValue(.init(data: nil, etag: result.etag, status: .OK, errorCode: nil))
                    } else if result.statusCode != 200 {
                        if #available(iOS 14.0, *) {
                            logger.debug(
                                "Failed to load \(url, privacy: .public): \(result.statusCode, privacy: .public), \(result.data.map { String(data: $0, encoding: .utf8)?.prefix(1024) ?? "?" } ?? "?")")
                        }
                        promise.setValue(.init(data: nil, etag: result.etag, status: .ERROR_NETWORK, errorCode: "\(result.statusCode)"))
                    } else if let data = result.data {
                        do {
                            if self.isRasterDebugModeEnabled,
                                let uiImage = UIImage(data: data)
                            {
                                let renderer = UIGraphicsImageRenderer(size: uiImage.size)
                                let img = renderer.image { ctx in
                                    self.applyDebugWatermark(url: urlString, byteCount: data.count, image: uiImage, wasCached: result.wasCached == true, ctx: ctx)
                                }
                                if let cgImage = img.cgImage,
                                    let textureHolder = try? await TextureHolder(cgImage)
                                {
                                    promise.setValue(.init(data: textureHolder, etag: result.etag, status: .OK, errorCode: nil))
                                    return
                                }
                            }

                            let textureHolder = try await TextureHolder(data)
                            promise.setValue(.init(data: textureHolder, etag: result.etag, status: .OK, errorCode: nil))
                        } catch TextureHolderError.emptyData {
                            promise.setValue(.init(data: nil, etag: result.etag, status: .OK, errorCode: nil))
                        } catch {
                            // If metal can not load this image
                            // try workaround to first load it into UIImage context
                            guard let uiImage = UIImage(data: data) else {
                                promise.setValue(.init(data: nil, etag: result.etag, status: .ERROR_OTHER, errorCode: "MNL"))
                                return
                            }

                            let renderer = UIGraphicsImageRenderer(size: uiImage.size)
                            let img = renderer.image { ctx in
                                if self.isRasterDebugModeEnabled {
                                    self.applyDebugWatermark(url: urlString, byteCount: data.count, image: uiImage, wasCached: result.wasCached == true, ctx: ctx)
                                } else {
                                    uiImage.draw(in: .init(origin: .init(), size: uiImage.size))
                                }
                            }

                            guard let cgImage = img.cgImage,
                                let textureHolder = try? await TextureHolder(cgImage)
                            else {
                                promise.setValue(.init(data: nil, etag: result.etag, status: .ERROR_OTHER, errorCode: "UINL"))
                                return
                            }

                            promise.setValue(.init(data: textureHolder, etag: result.etag, status: .OK, errorCode: nil))

                        }
                    } else {
                        promise.setValue(.init(data: nil, etag: result.etag, status: .ERROR_OTHER, errorCode: "\(result.statusCode)"))
                    }
                }

            }
        }

        taskQueue.sync {
            tasks[urlString] = task
        }

        return promise.getFuture()
    }

    open func loadData(_ url: String, etag: String?) -> MCDataLoaderResult {
        let semaphore = DispatchSemaphore(value: 0)
        var result: MCDataLoaderResult? = nil
        let uuid = UUID()
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

        let task = load(url: url) { [weak self] result in

            guard let self else { return }

            self.taskQueue.sync {
                _ = self.tasks.removeValue(forKey: urlString) == nil
            }

            switch result {
            case .failure(.timeout):
                if #available(iOS 14.0, *) {
                    logger.debug("Failed to load \(url, privacy: .public): Timeout")
                }
                promise.setValue(.init(data: nil, etag: nil, status: .ERROR_TIMEOUT, errorCode: nil))
            case .failure(.cancelled):
                // Do nothing, since the result is dropped anyway (setting a LoaderStatus will cause the SharedLib to do further computing)
                break
            case .failure(_):
                promise.setValue(.init(data: nil, etag: nil, status: .ERROR_OTHER, errorCode: "UNKN"))
            case .success(let result):
                if result.statusCode == 404 {
                    if #available(iOS 14.0, *) {
                        logger.debug("Failed to load \(url, privacy: .public): 404, \(result.data.map { String(data: $0, encoding: .utf8)?.prefix(1024) ?? "?" } ?? "?")")
                    }
                    promise.setValue(.init(data: nil, etag: result.etag, status: .ERROR_404, errorCode: "\(result.statusCode)"))

                } else if result.statusCode == 400 {
                    if #available(iOS 14.0, *) {
                        logger.debug("Failed to load \(url, privacy: .public): 400, \(result.data.map { String(data: $0, encoding: .utf8)?.prefix(1024) ?? "?" } ?? "?")")
                    }
                    promise.setValue(.init(data: nil, etag: result.etag, status: .ERROR_400, errorCode: "\(result.statusCode)"))
                } else if result.statusCode == 204 {
                    promise.setValue(.init(data: nil, etag: result.etag, status: .OK, errorCode: nil))
                } else if result.statusCode != 200 {
                    if #available(iOS 14.0, *) {
                        logger.debug(
                            "Failed to load \(url, privacy: .public): \(result.statusCode, privacy: .public), \(result.data.map { String(data: $0, encoding: .utf8)?.prefix(1024) ?? "?" } ?? "?")")
                    }
                    promise.setValue(.init(data: nil, etag: result.etag, status: .ERROR_NETWORK, errorCode: "\(result.statusCode)"))
                } else if let data = result.data {
                    promise.setValue(.init(data: data, etag: result.etag, status: .OK, errorCode: nil))
                } else {
                    promise.setValue(.init(data: nil, etag: result.etag, status: .ERROR_OTHER, errorCode: "\(result.statusCode)"))
                }
            }

        }

        taskQueue.sync {
            tasks[urlString] = task
        }

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

        let attrs: [NSAttributedString.Key: Any] = [
            NSAttributedString.Key.paragraphStyle: paragraphStyle,
            NSAttributedString.Key.backgroundColor: wasCached ? UIColor.lightGray.cgColor : UIColor.white.cgColor,
        ]

        let byteCountString = ByteCountFormatter().string(fromByteCount: Int64(byteCount))
        let loadedString = wasCached ? "Loaded from Cache" : "Loaded from www"

        let string = url + "\n" + loadedString + " at: " + ISO8601DateFormatter().string(from: .init()) + "\nSize: " + byteCountString
        string.draw(with: .init(origin: .init(x: 0, y: 25), size: size).inset(by: .init(top: 5, left: 5, bottom: 5, right: 5)), options: .usesLineFragmentOrigin, attributes: attrs, context: nil)
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

extension Int? {
    public var stringOrNil: String {
        switch self {
        case .none:
            return ""
        case let .some(wrapped):
            return "\(wrapped)"
        }
    }
}
