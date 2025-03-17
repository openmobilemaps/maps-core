/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import UIKit

extension UIScreen {
    // taken from https://github.com/Clafou/DevicePpi
    private static var mapping: [([String], Double)] = [
        (
            [
                // iPhone 13 mini
                "iPhone14,4",
                // iPhone 12 mini
                "iPhone13,1",
            ],
            476
        ),
        (
            [
                // iPhone 15
                "iPhone15,4",
                // iPhone 15 Plus
                "iPhone15,5",
                // iPhone 15 Pro
                "iPhone16,1",
                // iPhone 15 Pro Max
                "iPhone16,2",
                // iPhone 14
                "iPhone14,7",
                // iPhone 14 Pro
                "iPhone15,2",
                // iPhone 14 Pro Max
                "iPhone15,3",
                // iPhone 13
                "iPhone14,5",
                // iPhone 13 Pro
                "iPhone14,2",
                // iPhone 12
                "iPhone13,2",
                // iPhone 12 Pro
                "iPhone13,3",
            ],
            460
        ),
        (
            [
                // iPhone 14 Plus
                "iPhone14,8",
                // iPhone 13 Pro Max
                "iPhone14,3",
                // iPhone 12 Pro Max
                "iPhone13,4",
                // iPhone 11 Pro
                "iPhone12,3",
                // iPhone 11 Pro Max
                "iPhone12,5",
                // iPhone XS
                "iPhone11,2",
                // iPhone XS Max
                "iPhone11,4", "iPhone11,6",
                // iPhone X
                "iPhone10,3", "iPhone10,6",
            ],
            458
        ),
        (
            [
                // iPhone 8 Plus
                "iPhone10,2", "iPhone10,5",
                // iPhone 7 Plus
                "iPhone9,2", "iPhone9,4",
                // iPhone 6S Plus
                "iPhone8,2",
                // iPhone 6 Plus
                "iPhone7,1",
            ],
            401
        ),
        (
            [
                // iPhone 11
                "iPhone12,1",
                // iPhone XR
                "iPhone11,8",

                // iPhone SE (3rd generation)
                "iPhone14,6",
                // iPhone SE (2nd generation)
                "iPhone12,8",
                // iPhone 8
                "iPhone10,1", "iPhone10,4",
                // iPhone 7
                "iPhone9,1", "iPhone9,3",
                // iPhone 6S
                "iPhone8,1",
                // iPhone 6
                "iPhone7,2",

                // iPhone SE
                "iPhone8,4",
                // iPhone 5S
                "iPhone6,1", "iPhone6,2",
                // iPhone 5C
                "iPhone5,3", "iPhone5,4",
                // iPhone 5
                "iPhone5,1", "iPhone5,2",
                // iPod touch (7th generation)
                "iPod9,1",
                // iPod touch (6th generation)
                "iPod7,1",
                // iPod touch (5th generation)
                "iPod5,1",

                // iPhone 4S
                "iPhone4,1",

                // iPad mini (6th generation)
                "iPad14,1", "iPad14,2",
                // iPad mini (5th generation)
                "iPad11,1", "iPad11,2",
                // iPad mini 4
                "iPad5,1", "iPad5,2",
                // iPad mini 3
                "iPad4,7", "iPad4,8", "iPad4,9",
                // iPad mini 2
                "iPad4,4", "iPad4,5", "iPad4,6",
            ],
            326
        ),
        (
            [
                // iPad Air 11-inch (M2)
                "iPad14,8", "iPad14,9",
                // iPad Air 13-inch (M2)
                "iPad14,10", "iPad14,11",
                // iPad Pro 11-inch (M4)
                "iPad16,3", "iPad16,4",
                "iPad16,3-A", "iPad16,3-B", "iPad16,4-A", "iPad16,4-B",
                // iPad Pro 13-inch (M4)
                "iPad16,5", "iPad16,6",
                "iPad16,5-A", "iPad16,5-B", "iPad16,6-A", "iPad16,6-B",
                // iPad (10th generation)
                "iPad13,18", "iPad13,19",
                // iPad Pro (11″, 4th generation)
                "iPad14,3", "iPad14,4",
                "iPad14,3-A", "iPad14,3-B", "iPad14,4-A", "iPad14,4-B",
                // iPad Pro (12.9″, 6th generation)
                "iPad14,5", "iPad14,6",
                "iPad14,5-A", "iPad14,5-B", "iPad14,6-A", "iPad14,6-B",
                // iPad Air (5th generation)
                "iPad13,16", "iPad13,17",
                // iPad (9th generation)
                "iPad12,1", "iPad12,2",
                // iPad Pro (12.9″, 5th generation)
                "iPad13,8", "iPad13,9", "iPad13,10", "iPad13,11",
                // iPad Pro (11″, 3rd generation)
                "iPad13,4", "iPad13,5", "iPad13,6", "iPad13,7",
                // iPad Air (4th generation)
                "iPad13,1", "iPad13,2",
                // iPad (8th generation)
                "iPad11,6", "iPad11,7",
                // iPad Pro (12.9″, 4th generation)
                "iPad8,11", "iPad8,12",
                // iPad Pro (11″, 2nd generation)
                "iPad8,9", "iPad8,10",
                // iPad (7th generation)
                "iPad7,11", "iPad7,12",
                // iPad Air (3rd generation)
                "iPad11,3", "iPad11,4",
                // iPad Pro (12.9″, 3rd generation)
                "iPad8,5", "iPad8,6", "iPad8,7", "iPad8,8",
                // iPad Pro (11″)
                "iPad8,1", "iPad8,2", "iPad8,3", "iPad8,4",
                // iPad (6th generation)
                "iPad7,5", "iPad7,6",
                // iPad Pro (10.5″)
                "iPad7,3", "iPad7,4",
                // iPad Pro (12.9″, 2nd generation)
                "iPad7,1", "iPad7,2",
                // iPad (5th generation)
                "iPad6,11", "iPad6,12",
                // iPad Pro (12.9″)
                "iPad6,7", "iPad6,8",
                // iPad Pro (9.7″)
                "iPad6,3", "iPad6,4",
                // iPad Air 2
                "iPad5,3", "iPad5,4",
                // iPad Air
                "iPad4,1", "iPad4,2", "iPad4,3",
                // iPad (4th generation)
                "iPad3,4", "iPad3,5", "iPad3,6",
                // iPad (3rd generation)
                "iPad3,1", "iPad3,2", "iPad3,3",
            ],
            264
        ),
        (
            [
                // iPad mini
                "iPad2,5", "iPad2,6", "iPad2,7",
            ],
            163
        ),
        (
            [
                // iPad 2
                "iPad2,1", "iPad2,2", "iPad2,3", "iPad2,4",
            ],
            132
        ),
    ]

    static let pixelsPerInch: Double = {
        let modelIdentifier = UIDevice.modelIdentifier
        if let ppi = mapping.first(where: { $0.0.contains(modelIdentifier) }) {
            return ppi.1
        }
        let screen = UIScreen.main
        let idiom = UIDevice.current.userInterfaceIdiom
        if idiom == .pad {
            return screen.scale == 2 ? 264 : 132
        }
        if screen.scale == 3 {
            return screen.nativeScale == 3 ? 460 : 401
        }
        return 326
    }()

    var pointsPerInch: Double {
        Self.pixelsPerInch / nativeScale
    }
}

private extension UIDevice {
    // model identifiers can be found at https://www.theiphonewiki.com/wiki/Models
    static let modelIdentifier: String = {
        if let simulatorModelIdentifier = ProcessInfo().environment["SIMULATOR_MODEL_IDENTIFIER"] { return simulatorModelIdentifier }
        var sysinfo = utsname()
        uname(&sysinfo)  // ignore return value
        return String(bytes: Data(bytes: &sysinfo.machine, count: Int(_SYS_NAMELEN)), encoding: .ascii)!.trimmingCharacters(in: .controlCharacters)
    }()
}
