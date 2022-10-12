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

public class DataHolder: MCDataHolderInterface {
    private var data: Data

    public init(data: Data) {
        self.data = data
    }

    public func getData() -> Data {
        data
    }
}

open class MCTextureLoader: MCLoaderInterface {

    private let session: URLSession

    public var isRasterDebugModeEnabled: Bool

    public init(urlSession: URLSession? = nil)
    {
        if let urlSession = urlSession {
            session = urlSession
        } else {
            let sessionConfig = URLSessionConfiguration.default
            sessionConfig.urlCache = URLCache(memoryCapacity: 100 * 1024 * 1024, diskCapacity: 500 * 1024 * 1024, diskPath: "ch.openmobilemaps.urlcache")
            sessionConfig.networkServiceType = .responsiveData
            session = .init(configuration: sessionConfig)
        }

        isRasterDebugModeEnabled = UserDefaults.standard.bool(forKey: "io.openmobilemaps.debug.rastertiles.enabled")
    }

    open func loadSync(request: URLRequest) -> (Data?, HTTPURLResponse?, NSError?) {

        let semaphore = DispatchSemaphore(value: 0)

        var result: Data?
        var response: HTTPURLResponse?
        var error: NSError?

        var task = session.dataTask(with: request) { data, response_, error_ in
            result = data
            response = response_ as? HTTPURLResponse
            error = error_ as NSError?
            semaphore.signal()
        }

        modifyDataTask(task: &task)

        task.resume()
        semaphore.wait()

        return (result, response, error)

    }

    open func loadTexture(_ url: String, etag: String?) -> MCTextureLoaderResult {
        let urlString = url
        guard let url = URL(string: urlString) else {
            preconditionFailure("invalid url: \(urlString)")
        }

        var urlRequest = URLRequest(url: url)

        modifyUrlRequest(request: &urlRequest)

        var wasCached = false;
        if isRasterDebugModeEnabled,
           session.configuration.urlCache?.cachedResponse(for: urlRequest) != nil {
            wasCached = true
        }

        let (result, response, error) = loadSync(request: urlRequest)

        if error?.domain == NSURLErrorDomain, error?.code == NSURLErrorTimedOut {
            return .init(data: nil, etag: response?.etag, status: .ERROR_TIMEOUT, errorCode: (error?.code).stringOrNil)
        }

        if response?.statusCode == 404 {
            return .init(data: nil, etag: response?.etag, status: .ERROR_404, errorCode: (response?.statusCode).stringOrNil)
        } else if response?.statusCode == 400 {
            return .init(data: nil, etag: response?.etag, status: .ERROR_400, errorCode: (response?.statusCode).stringOrNil)
        } else if response?.statusCode != 200 {
            return .init(data: nil, etag: response?.etag, status: .ERROR_NETWORK, errorCode: (response?.statusCode).stringOrNil)
        }

        guard let data = result else {
            return .init(data: nil, etag: response?.etag, status: .ERROR_OTHER, errorCode: (response?.statusCode).stringOrNil)
        }

        do {
            if isRasterDebugModeEnabled,
                let uiImage = UIImage(data: data) {
                let renderer = UIGraphicsImageRenderer(size: uiImage.size)
                let img = renderer.image { ctx in
                    self.applyDebugWatermark(url: urlString, byteCount: data.count, image: uiImage, wasCached: wasCached, ctx: ctx)
                }
                if let cgImage = img.cgImage,
                      let textureHolder = try? TextureHolder(cgImage) {
                    return .init(data: textureHolder, etag: response?.etag, status: .OK, errorCode: nil)
                }
            }

            let textureHolder = try TextureHolder(data)
            return .init(data: textureHolder, etag: response?.etag, status: .OK, errorCode: nil)
        } catch TextureHolderError.emptyData {
            return .init(data: nil, etag: response?.etag, status: .OK, errorCode: nil)
        } catch {
            // If metal can not load this image
            // try workaround to first load it into UIImage context
            guard let uiImage = UIImage(data: data) else {
                return .init(data: nil, etag: response?.etag, status: .ERROR_OTHER, errorCode: "MNL")
            }

            let renderer = UIGraphicsImageRenderer(size: uiImage.size)
            let img = renderer.image { ctx in
                if isRasterDebugModeEnabled {
                    self.applyDebugWatermark(url: urlString, byteCount: data.count, image: uiImage, wasCached: wasCached, ctx: ctx)
                } else {
                    uiImage.draw(in: .init(origin: .init(), size: uiImage.size))
                }
            }

            guard let cgImage = img.cgImage,
                  let textureHolder = try? TextureHolder(cgImage) else {
                return .init(data: nil, etag: response?.etag, status: .ERROR_OTHER, errorCode: "UINL")
            }

            return .init(data: textureHolder, etag: response?.etag, status: .OK, errorCode: nil)
        }
    }

    open func loadData(_ url: String, etag: String?) -> MCDataLoaderResult {
        let urlString = url
        guard let url = URL(string: urlString) else {
            preconditionFailure("invalid url: \(urlString)")
        }

        let semaphore = DispatchSemaphore(value: 0)

        var result: Data?
        var response: HTTPURLResponse?
        var error: NSError?

        var urlRequest = URLRequest(url: url)

        modifyUrlRequest(request: &urlRequest)

        var task = session.dataTask(with: urlRequest) { data, response_, error_ in
            result = data
            response = response_ as? HTTPURLResponse
            error = error_ as NSError?
            semaphore.signal()
        }

        modifyDataTask(task: &task)

        task.resume()
        semaphore.wait()

        if error?.domain == NSURLErrorDomain, error?.code == NSURLErrorTimedOut {
            return .init(data: nil, etag: response?.etag, status: .ERROR_TIMEOUT, errorCode: (error?.code).stringOrNil)
        }

        if response?.statusCode == 404 {
            return .init(data: nil, etag: response?.etag, status: .ERROR_404, errorCode: (response?.statusCode).stringOrNil)
        } else if response?.statusCode == 400 {
            return .init(data: nil, etag: response?.etag, status: .ERROR_400, errorCode: (response?.statusCode).stringOrNil)
        } else if response?.statusCode != 200 {
            return .init(data: nil, etag: response?.etag, status: .ERROR_NETWORK, errorCode: (response?.statusCode).stringOrNil)
        }

        guard let data = result else {
            return .init(data: nil, etag: response?.etag, status: .ERROR_OTHER, errorCode: (response?.statusCode).stringOrNil)
        }

        return .init(data: DataHolder(data: data), etag: response?.etag, status: .OK, errorCode: nil)
    }

    open func modifyUrlRequest(request _: inout URLRequest) {
    }

    open func modifyDataTask(task _: inout URLSessionDataTask) {
    }

    func applyDebugWatermark(url: String, byteCount: Int, image: UIImage, wasCached: Bool , ctx: UIGraphicsRendererContext) {
        let size = image.size

        image.draw(in: .init(origin: .init(), size: size))

        guard isRasterDebugModeEnabled else { return }

        ctx.cgContext.setFillColor(gray: 0.1, alpha: 0.2)
        ctx.fill( .init(origin: .init(), size: size), blendMode: .normal)

        ctx.cgContext.setStrokeColor(UIColor.black.cgColor)
        ctx.cgContext.setLineWidth(5.0)
        ctx.cgContext.stroke(.init(origin: .init(), size: size).inset(by: .init()))

        let paragraphStyle = NSMutableParagraphStyle()
        paragraphStyle.alignment = .center

        let attrs: [NSAttributedString.Key : Any] = [NSAttributedString.Key.paragraphStyle: paragraphStyle,
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

private extension Optional where Wrapped == Int {
    var stringOrNil: String {
        switch self {
            case .none:
                return ""
            case let .some(wrapped):
                return "\(wrapped)"
        }
    }
}
