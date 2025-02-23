//
//  DataProvider.swift
//  MapCore
//
//  Created by Nicolas Märki on 21.02.2025.
//

import MapCore

class DataProvider: MCTextureLoader, @unchecked Sendable {

    enum InputData {
        case styleJson(String)
        case baseStyleURL(String)
        case none
    }

    let inputData: InputData

    static let styleJsonPlaceholder = "style.json"

    init() {
        self.inputData = .none
    }

    init(styleJson: String) {
        self.inputData = .styleJson(styleJson)
    }

    init(_ baseStyleURL: String) {
        self.inputData = .baseStyleURL(baseStyleURL)
    }

    init(_ vectorStyle: VectorStyle) {
        let styleJson = String(data: (try! JSONEncoder().encode(vectorStyle)), encoding: .utf8)!
        self.inputData = .styleJson(styleJson)
    }

    override func load(url urlString: String, setWasCachedFlag: Bool = false, completion: @escaping (Result<MCTextureLoader.LoaderResult, MCTextureLoader.LoaderResult.LoaderError>) -> Void) -> (
        any CancellableTask
    )? {

        if urlString == Self.styleJsonPlaceholder {
            switch inputData {
            case .styleJson(let styleJson):
                completion(.success(.init(data: styleJson.data(using: .utf8), statusCode: 200, etag: nil, wasCached: true)))
            case .baseStyleURL(let baseStyleURL):
                Task {
                    do {
                        let data = try await URLSession.shared.data(from: URL(string: baseStyleURL)!).0
                        completion(.success(.init(data: data, statusCode: 200, etag: nil, wasCached: true)))
                    } catch {
                        completion(.failure(.other(error)))
                    }
                }
                return nil
            case .none: break
            }
        }

        return super.load(url: urlString, setWasCachedFlag: setWasCachedFlag, completion: completion)
    }

}
