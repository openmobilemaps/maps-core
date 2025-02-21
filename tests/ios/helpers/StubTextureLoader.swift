//
//  StubTextureLoader.swift
//  MapCore
//
//  Created by Nicolas Märki on 21.02.2025.
//

import MapCore

class StubTextureLoader: MCTextureLoader, @unchecked Sendable {

    override func load(url urlString: String, setWasCachedFlag: Bool = false, completion: @escaping (Result<MCTextureLoader.LoaderResult, MCTextureLoader.LoaderResult.LoaderError>) -> Void) -> (any CancellableTask)? {
        super.load(url: urlString, setWasCachedFlag: setWasCachedFlag, completion: completion)
    }

}

