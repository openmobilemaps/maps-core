//
//  AsyncLayer.swift
//
//
//  Created by Nicolas Märki on 15.02.2024.
//

import Foundation

@available(iOS 13.0, *)
public final class AsyncLayer: Layer, ObservableObject, @unchecked Sendable {
    private(set) public var error: Error?
    private(set) public var isLoading = true

    private(set) public var baseLayer: Layer?

    public var interface: MCLayerInterface? { baseLayer?.interface }

    public init(setup: @escaping @Sendable () async throws -> Layer) {
        Task {
            do {
                self.baseLayer = try await setup()
            }
            catch {
                self.error = error
            }
            self.isLoading = false
            await MainActor.run {
                objectWillChange.send()
            }
        }
    }

    
}
