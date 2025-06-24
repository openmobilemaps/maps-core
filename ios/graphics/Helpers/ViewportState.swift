/*
 * Copyright (c) 2025 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import Atomics

final class ViewportState {
    private let packedViewport = ManagedAtomic<UInt64>(0)

    // MARK: - Writing

    public func setViewportSize(x: Int32, y: Int32) {
        store(x: x, y: y)
    }

    // MARK: - Reading

    public var aspectRatio: Float {
        let (x, y) = load()
        return Float(x) / Float(y)
    }

    public var viewportSize: MCVec2I {
        let (x, y) = load()
        return MCVec2I(x: x, y: y)
    }

    // MARK: - Helpers

    private func store(x: Int32, y: Int32) {
        let ux = UInt64(UInt32(bitPattern: x))
        let uy = UInt64(UInt32(bitPattern: y))
        packedViewport.store((ux << 32) | uy, ordering: .relaxed)
    }

    private func load() -> (Int32, Int32) {
        let packed = packedViewport.load(ordering: .relaxed)
        let x = Int32(bitPattern: UInt32(packed >> 32))
        let y = Int32(bitPattern: UInt32(packed & 0xFFFFFFFF))
        return (x, y)
    }
}
