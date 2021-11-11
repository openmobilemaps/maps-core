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

public class StaticMetalLibrary<Key: Hashable, Value>: Library {
    private var storage: [Key: Value]

    init(_ allKeys: [Key], _ newInstance: (Key) throws -> Value) rethrows {
        var collector: [Key: Value] = [:]
        for key in allKeys {
            collector[key] = try newInstance(key)
        }
        storage = collector
    }

    public final func register(_ value: Value, for key: Key) {
        storage[key] = value
    }

    public final func value(_ key: Key) -> Value {
        storage[key]!
    }

    public final subscript(_ key: Key) -> Value {
        value(key)
    }
}
