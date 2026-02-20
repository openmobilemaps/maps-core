/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import MapCoreSharedModule
@preconcurrency import Metal
import UIKit

extension MTLDevice {
    func makeBuffer(from sharedBytes: MCSharedBytes) -> MTLBuffer? {
        guard let pointer = UnsafeRawPointer(bitPattern: Int(sharedBytes.address)),
            sharedBytes.elementCount > 0
        else { return nil }

        return self.makeBuffer(bytes: pointer, length: Int(sharedBytes.elementCount * sharedBytes.bytesPerElement), options: [])
    }

    func makeBuffer(from ownedBytes: MCOwnedBytes) -> MTLBuffer? {
        defer {
            MCOwnedBytesDestructor.free(ownedBytes)
        }

        guard let pointer = UnsafeRawPointer(bitPattern: Int(ownedBytes.address)),
            ownedBytes.elementCount > 0
        else { return nil }

        let buffer = self.makeBuffer(bytes: pointer, length: Int(ownedBytes.elementCount * ownedBytes.bytesPerElement), options: [])
        MCOwnedBytesDestructor.free(ownedBytes);
        return buffer;

        return self.makeBuffer(bytes: pointer, length: Int(ownedBytes.elementCount * ownedBytes.bytesPerElement), options: [])
        /*
        // This copy-free creation of the buffer object might be nice, but
        // requires to use mmap/munmap directly instead of malloc/free.
        // We would also need to be more careful; overallocations during
        // creation of the data (e.g in LineGeometryBuilder) would no longer be
        // temporary, but hurt for the entire lifetime of this buffer.
        // This is all feasible, but unclear if its worth the additional code.
        guard let pointer = UnsafeMutableRawPointer(bitPattern: Int(ownedBytes.address)),
            ownedBytes.elementCount > 0
        else { return nil }
        return self.makeBuffer(
            bytesNoCopy: pointer,
            length: Int(ownedBytes.elementCount * ownedBytes.bytesPerElement), options: [],
            deallocator: { (UnsafeMutableRawPointer, Int) -> Void in
                MCOwnedBytesDestructor.free(ownedBytes);
            })
        */
    }
}

extension MTLBuffer {
    func copyMemory(from sharedBytes: MCSharedBytes) {
        if let p = UnsafeRawPointer(bitPattern: Int(sharedBytes.address)),
            sharedBytes.elementCount > 0
        {
            self.contents().copyMemory(from: p, byteCount: Int(sharedBytes.elementCount * sharedBytes.bytesPerElement))
        }
    }

    func copyMemory(bytes pointer: UnsafeRawPointer, length: Int) {
        self.contents().copyMemory(from: pointer, byteCount: length)
    }
}

public extension MTLBuffer? {
    mutating func copyOrCreate(from sharedBytes: MCSharedBytes, device: MTLDevice) {
        switch self {
            case .none:
                self = device.makeBuffer(from: sharedBytes)
            case let .some(wrapped):
                if wrapped.length == Int(sharedBytes.elementCount * sharedBytes.bytesPerElement) {
                    wrapped.copyMemory(from: sharedBytes)
                } else {
                    self = device.makeBuffer(from: sharedBytes)
                }
        }
    }

    mutating func copyOrCreate(bytes pointer: UnsafeRawPointer, length: Int, device: MTLDevice) {
        switch self {
            case .none:
                self = device.makeBuffer(bytes: pointer, length: length)
            case let .some(wrapped):
                if wrapped.length >= length {
                    wrapped.copyMemory(bytes: pointer, length: length)
                } else {
                    self = device.makeBuffer(bytes: pointer, length: length)
                }
        }
    }
}
