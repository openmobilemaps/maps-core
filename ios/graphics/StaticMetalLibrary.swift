/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import Foundation

protocol Library {
    associatedtype Key: Hashable
    associatedtype Value
}

enum LibraryError: Error {
    case invalidKey
}

public struct StaticMetalLibrary<Key: Hashable & Sendable, Value: Sendable>: Library, Sendable {
    private var storage: [Key: Value]

    init(_ allKeys: [Key], _ newInstance: (Key) throws -> Value) rethrows {
        var collector: [Key: Value] = [:]
        for key in allKeys {
            collector[key] = try newInstance(key)
        }
        storage = collector
    }

    public mutating func register(_ value: Value, for key: Key) {
        storage[key] = value
    }

    public func value(_ key: Key) -> Value? {
        storage[key]
    }

    public subscript(_ key: Key) -> Value? {
        value(key)
    }
}
