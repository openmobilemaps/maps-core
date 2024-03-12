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
import Metal
import UIKit

extension MTLDevice {
    func makeBuffer(from sharedBytes: MCSharedBytes) -> MTLBuffer? {
        guard let pointer = UnsafeRawPointer(bitPattern: Int(sharedBytes.address)),
              sharedBytes.elementCount > 0
        else { return nil }

        return self.makeBuffer(bytes: pointer, length: Int(sharedBytes.elementCount * sharedBytes.bytesPerElement), options: [])
    }
}

extension MTLBuffer {
    func copyMemory(from sharedBytes: MCSharedBytes) {
        if let p = UnsafeRawPointer(bitPattern: Int(sharedBytes.address)),
           sharedBytes.elementCount > 0 {
            self.contents().copyMemory(from: p, byteCount: Int(sharedBytes.elementCount * sharedBytes.bytesPerElement))
        }
    }
}

extension MTLBuffer? {
    mutating public func copyOrCreate(from sharedBytes: MCSharedBytes, device: MTLDevice) {
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
}
