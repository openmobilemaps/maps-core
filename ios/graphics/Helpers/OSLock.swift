//
//  OSLock.swift
//  
//
//  Created by Stefan Mitterrutzner on 01.12.22.
//

import Foundation
import os.lock

// Wrapper around os_unfair_lock_t
public class OSLock {
    func lock() {
        os_unfair_lock_lock(oslock)
    }

    func unlock() {
        os_unfair_lock_unlock(oslock)
    }

    func trylock() -> Bool {
        return os_unfair_lock_trylock(oslock)
    }

    func withCritical<Result>(_ section: () throws -> Result) rethrows -> Result {
        if !os_unfair_lock_trylock(oslock) {
            os_unfair_lock_lock(oslock)
        }

        defer {
            os_unfair_lock_unlock(oslock)
        }

        return try section()
    }

    private let oslock: UnsafeMutablePointer<os_unfair_lock_s> = {
        let lock = os_unfair_lock_t.allocate(capacity: 1)
        lock.initialize(to: os_unfair_lock_s())
        return lock
    }()

    deinit {
        oslock.deinitialize(count: 1)
        oslock.deallocate()
    }
}
