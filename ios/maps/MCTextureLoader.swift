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

public class MCTextureLoader: MCTextureLoaderInterface {
    private let session: URLSession

    public init() {
        let sessionConfig = URLSessionConfiguration.default
        sessionConfig.urlCache = URLCache(memoryCapacity: 100 * 1024 * 1024, diskCapacity: 500 * 1024 * 1024, diskPath: "ch.openmobilemaps.urlcache")
        sessionConfig.networkServiceType = .responsiveData
        session = .init(configuration: sessionConfig)
    }

    public func loadTexture(_ url: String) -> MCTextureLoaderResult {
        let urlString = url
        guard let url = URL(string: urlString) else {
            preconditionFailure("invalid url: \(urlString)")
        }

        let semaphore = DispatchSemaphore(value: 0)

        var result: Data?
        var response: HTTPURLResponse?
        var error: NSError?

        let task = session.dataTask(with: url) { data, response_, error_ in
            result = data
            response = response_ as? HTTPURLResponse
            error = error_ as NSError?
            semaphore.signal()
        }

        task.resume()
        semaphore.wait()

        if error?.domain == NSURLErrorDomain, error?.code == NSURLErrorTimedOut {
            return .init(data: nil, status: .ERROR_TIMEOUT)
        }

        if response?.statusCode == 404 {
            return .init(data: nil, status: .ERROR_404)
        } else if response?.statusCode == 400 {
            return .init(data: nil, status: .ERROR_400)
        } else if response?.statusCode != 200 {
            return .init(data: nil, status: .ERROR_NETWORK)
        }

        guard let data = result else {
            return .init(data: nil, status: .ERROR_OTHER)
        }

        if url.pathExtension == "png" {
            guard let uiImage = UIImage(data: data) else {
                return .init(data: nil, status: .ERROR_OTHER)
            }

            let renderer = UIGraphicsImageRenderer(size: uiImage.size)
            let img = renderer.image { _ in
                uiImage.draw(in: .init(origin: .init(), size: uiImage.size))
            }

            guard let cgImage = img.cgImage,
                  let textureHolder = try? TextureHolder(cgImage) else {
                return .init(data: nil, status: .ERROR_OTHER)
            }

            return .init(data: textureHolder, status: .OK)
        } else {
            guard let textureHolder = try? TextureHolder(data) else {
                return .init(data: nil, status: .ERROR_OTHER)
            }
            return .init(data: textureHolder, status: .OK)
        }
    }
}
