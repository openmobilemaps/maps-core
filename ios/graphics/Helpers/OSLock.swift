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

    let oslock = {
        let lock1 = UnsafeMutablePointer<os_unfair_lock>.allocate(capacity: 1)
        lock1.initialize(to: .init())
        return lock1
    }()

    deinit {
        oslock.deinitialize(count: 1)
        oslock.deallocate()
    }
}
