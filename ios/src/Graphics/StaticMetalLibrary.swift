//
//  StaticMetalLibrary.swift
//  SwisstopoShared
//
//  Created by Joseph El Mallah on 12.02.20.
//  Copyright © 2020 Ubique Innovation AG. All rights reserved.
//

import Foundation

protocol Library {
    associatedtype Key: Hashable
    associatedtype Value
}

class StaticMetalLibrary<Key: Hashable & CaseIterable, Value>: Library {
    private var storage: [Key: Value]

    init(_ newInstance: (Key) throws -> Value) rethrows {
        var collector: [Key: Value] = [:]
        for key in Key.allCases {
            collector[key] = try newInstance(key)
        }
        storage = collector
    }

    final func value(_ key: Key) -> Value {
        return storage[key]!
    }

    final subscript(_ key: Key) -> Value {
        value(key)
    }
}
