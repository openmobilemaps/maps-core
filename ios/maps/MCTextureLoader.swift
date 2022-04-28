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

    public init(urlSession: URLSession? = nil) {
        if let urlSession = urlSession {
            session = urlSession
        } else {
            let sessionConfig = URLSessionConfiguration.default
            sessionConfig.urlCache = URLCache(memoryCapacity: 100 * 1024 * 1024, diskCapacity: 500 * 1024 * 1024, diskPath: "ch.openmobilemaps.urlcache")
            sessionConfig.networkServiceType = .responsiveData
            session = .init(configuration: sessionConfig)
        }
    }

    open func loadTexture(_ url: String, etag: String?) -> MCTextureLoaderResult {
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
            return .init(data: nil, etag: response?.etag, status: .ERROR_TIMEOUT)
        }

        if response?.statusCode == 404 {
            return .init(data: nil, etag: response?.etag, status: .ERROR_404)
        } else if response?.statusCode == 400 {
            return .init(data: nil, etag: response?.etag, status: .ERROR_400)
        } else if response?.statusCode != 200 {
            return .init(data: nil, etag: response?.etag, status: .ERROR_NETWORK)
        }

        guard let data = result else {
            return .init(data: nil, etag: response?.etag, status: .ERROR_OTHER)
        }

        guard let textureHolder = try? TextureHolder(data) else {
            // If metal can not load this image
            // try workaround to first load it into UIImage context
            guard let uiImage = UIImage(data: data) else {
                return .init(data: nil, etag: response?.etag, status: .ERROR_OTHER)
            }

            let renderer = UIGraphicsImageRenderer(size: uiImage.size)
            let img = renderer.image { _ in
                uiImage.draw(in: .init(origin: .init(), size: uiImage.size))
            }

            guard let cgImage = img.cgImage,
                  let textureHolder = try? TextureHolder(cgImage) else {
                return .init(data: nil, etag: response?.etag, status: .ERROR_OTHER)
            }

            return .init(data: textureHolder, etag: response?.etag, status: .OK)
        }
        return .init(data: textureHolder, etag: response?.etag, status: .OK)
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
            return .init(data: nil, etag: response?.etag, status: .ERROR_TIMEOUT)
        }

        if response?.statusCode == 404 {
            return .init(data: nil, etag: response?.etag, status: .ERROR_404)
        } else if response?.statusCode == 400 {
            return .init(data: nil, etag: response?.etag, status: .ERROR_400)
        } else if response?.statusCode != 200 {
            return .init(data: nil, etag: response?.etag, status: .ERROR_NETWORK)
        }

        guard let data = result else {
            return .init(data: nil, etag: response?.etag, status: .ERROR_OTHER)
        }

        return .init(data: DataHolder(data: data), etag: response?.etag, status: .OK)
    }

    open func modifyUrlRequest(request _: inout URLRequest) {
    }

    open func modifyDataTask(task _: inout URLSessionDataTask) {
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
