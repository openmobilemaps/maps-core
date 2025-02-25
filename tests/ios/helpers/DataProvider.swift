//
//  DataProvider.swift
//  MapCore
//
//  Created by Nicolas Märki on 21.02.2025.
//

import MapCore
import CryptoKit

class DataProvider: MCTextureLoader, @unchecked Sendable {

    enum InputData {
        case styleJson(String)
        case baseStyleURL(String)
        case none
    }

    let inputData: InputData
    let filePath: String

    static let styleJsonPlaceholder = "{STYLE_JSON}"

    init(file filePath: StaticString = #file) {
        self.inputData = .none
        self.filePath = "\(filePath)"
    }

    init(styleJson: String, file filePath: StaticString = #file) {
        self.inputData = .styleJson(styleJson)
        self.filePath = "\(filePath)"
    }

    init(_ baseStyleURL: String, file filePath: StaticString = #file) {
        self.inputData = .baseStyleURL(baseStyleURL)
        self.filePath = "\(filePath)"
    }

    init(_ vectorStyle: VectorStyle, file filePath: StaticString = #file) {
        let styleJson = String(data: (try! JSONEncoder().encode(vectorStyle)), encoding: .utf8)!
        self.inputData = .styleJson(styleJson)
        self.filePath = "\(filePath)"
    }

    override func load(url urlString: String, setWasCachedFlag: Bool = false, completion: @escaping (Result<MCTextureLoader.LoaderResult, MCTextureLoader.LoaderResult.LoaderError>) -> Void) -> (
        any CancellableTask
    )? {

        var urlString = urlString

        if urlString == Self.styleJsonPlaceholder {
            switch inputData {
            case .styleJson(let styleJson):
                completion(.success(.init(data: styleJson.data(using: .utf8), statusCode: 200, etag: nil, wasCached: true)))
            case .baseStyleURL(let baseStyleURL):
                urlString = baseStyleURL
            case .none:
                break
            }
        }

        Task {
            do {
                let data = try await self.loadData(urlString: urlString)
                completion(.success(.init(data: data, statusCode: 200, etag: nil, wasCached: true)))
            } catch {
                completion(.failure(.other(error)))
            }
        }

        return nil
    }

    func loadData(urlString: String) async throws -> Data {
        let fileUrl = URL(fileURLWithPath: filePath, isDirectory: false)
        let snapshotsBaseUrl = fileUrl.deletingLastPathComponent()
        let snapshotDirectoryUrl = snapshotsBaseUrl.appendingPathComponent("__Requests__")

        guard let urlStringData = urlString.data(using: .utf8) else {
            throw NSError(domain: "Invalid URL string", code: 0, userInfo: nil)
        }
        let hash = SHA256.hash(data: urlStringData)
        let hex = hash.map { String(format: "%02x", $0) }.joined()

        guard let url = URL(string: urlString) else {
            throw NSError(domain: "Invalid URL string", code: 0, userInfo: nil)
        }

        let snapshotFileName = hex.prefix(10) + "-" + url.lastPathComponent

        let snapshotFileUrl = snapshotDirectoryUrl.appendingPathComponent(String(snapshotFileName))

        if FileManager.default.fileExists(atPath: snapshotFileUrl.path) {
            let data = try Data(contentsOf: snapshotFileUrl)
            if data.isEmpty {
                throw NSError(domain: "Invalid response", code: 0, userInfo: nil)
            }
            return data
        }

        let (data, response) = try await URLSession.shared.data(from: url)

        guard let response = response as? HTTPURLResponse, response.statusCode == 200 else {
            FileManager.default.createFile(atPath: snapshotFileUrl.path, contents: Data(), attributes: nil)
            throw NSError(domain: "Invalid response", code: 0, userInfo: nil)
        }

        try FileManager.default.createDirectory(at: snapshotDirectoryUrl, withIntermediateDirectories: true, attributes: nil)
        FileManager.default.createFile(atPath: snapshotFileUrl.path, contents: data, attributes: nil)

        return data
    }

}
