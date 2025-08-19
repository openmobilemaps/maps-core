//
//  AsyncLayer.swift
//
//
//  Created by Nicolas Märki on 15.02.2024.
//

import Foundation

public final class AsyncLayer: Layer, ObservableObject, @unchecked Sendable {
    private(set) public var error: Error?
    private(set) public var isLoading = true

    public private(set) var baseLayer: Layer?

    public var interface: MCLayerInterface? { baseLayer?.interface }

    public init(setup: @escaping @Sendable @MainActor () async throws -> Layer) {
        Task {
            do {
                self.baseLayer = try await setup()
            } catch {
                self.error = error
            }
            self.isLoading = false
            await MainActor.run {
                objectWillChange.send()
            }
        }
    }

    public var beforeAdding: ((MCLayerInterface, MCMapView) -> Void)? {
        baseLayer?.beforeAdding
    }
}
