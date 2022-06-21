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
    static let pixelsPerInch: Double = {
        switch UIDevice.modelIdentifier {
            case "iPhone14,4",  // iPhone 13 mini
                "iPhone13,1": // iPhone 12 mini
                return 476
            case "iPhone14,5", // iPhone 13
                "iPhone14,2", // iPhone 13 Pro
                "iPhone13,2", // iPhone 12
                "iPhone13,3": // iPhone 12 Pro
                return 460
            case "iPhone14,3", // iPhone 13 Pro Max
                "iPhone13,4", // iPhone 12 Pro Max
                "iPhone12,3", // iPhone 11 Pro
                "iPhone12,5", // iPhone 11 Pro Max
                "iPhone11,2", // iPhone XS
                "iPhone11,4", "iPhone11,6", // iPhone XS Max
                "iPhone10,3", "iPhone10,6": // iPhone X
                return 458
            case "iPhone10,2", "iPhone10,5", // iPhone 8 Plus
                "iPhone9,2", "iPhone9,4", // iPhone 7 Plus
                "iPhone8,2", // iPhone 6S Plus
                "iPhone7,1": // iPhone 6 Plus
                return 401
            case "iPhone12,1", // iPhone 11
                "iPhone11,8", // iPhone XR
                "iPhone14,6", // iPhone SE (3rd generation)
                "iPhone12,8", // iPhone SE (2nd generation)
                "iPhone10,1", "iPhone10,4", // iPhone 8
                "iPhone9,1", "iPhone9,3", // iPhone 7
                "iPhone8,1", // iPhone 6S
                "iPhone7,2", // iPhone 6
                "iPhone8,4", // iPhone SE
                "iPhone6,1", "iPhone6,2", // iPhone 5S
                "iPhone5,3", "iPhone5,4", // iPhone 5C
                "iPhone5,1", "iPhone5,2", // iPhone 5
                "iPod9,1", // iPod touch (7th generation)
                "iPod7,1", // iPod touch (6th generation)
                "iPod5,1", // iPod touch (5th generation)
                "iPhone4,1", // iPhone 4S
                "iPad14,1", "iPad14,2", // iPad mini (6th generation)
                "iPad11,1", "iPad11,2", // iPad mini (5th generation)
                "iPad5,1", "iPad5,2", // iPad mini 4
                "iPad4,7", "iPad4,8", "iPad4,9", // iPad mini 3
                "iPad4,4", "iPad4,5", "iPad4,6": // iPad mini 2
                return 326
            case "iPad13,16", "iPad13,17", // iPad Air (5th generation)
                "iPad12,1", "iPad12,2",  // iPad (9th generation)
                "iPad13,8", "iPad13,9", // iPad Pro (12.9″, 5th generation) "iPad13,10", "iPad13,11",
                "iPad13,4", "iPad13,5", "iPad13,6", "iPad13,7",  // iPad Pro (11″, 3rd generation)
                "iPad13,1", "iPad13,2",    // iPad Air (4th generation)
                "iPad11,6", "iPad11,7", // iPad (8th generation)
                "iPad8,11", "iPad8,12", // iPad Pro (12.9″, 4th generation)
                "iPad8,9", "iPad8,10", // iPad Pro (11″, 2nd generation)
                "iPad7,11", "iPad7,12", // iPad (7th generation),
                "iPad11,3", "iPad11,4", // iPad Air (3rd generation)
                "iPad8,5", "iPad8,6", "// iPad Pro (12.9″, 3rd generation)iPad8,7", "iPad8,8",
                "iPad8,1", "iPad8,2", "iPad8,3", "iPad8,4", // iPad Pro (11″)
                "iPad7,5", "iPad7,6",    // iPad (6th generation)
                "iPad7,3", "iPad7,4", // iPad Pro (10.5″)
                "iPad7,1", "iPad7,2", // iPad Pro (12.9″, 2nd generation)
                "iPad6,11", "iPad6,12// iPad (5th generation)",
                "iPad6,7", "iPad6,8",  // iPad Pro (12.9″)
                "iPad6,3", "iPad6,4", // iPad Pro (9.7″)
                "iPad5,3", "iPad5,4", // iPad Air 2
                "iPad4,1", "iPad4,2", // iPad Air "iPad4,3",
                "iPad3,4", "iPad3,5", "iPad3,6", // iPad (4th generation)
                "iPad3,1", "iPad3,2", "iPad3,3":// iPad (3rd generation)
                return 264
            case "iPad2,5", "iPad2,6", "iPad2,7": // iPad mini
                return 264
            default:
                if UIDevice.current.userInterfaceIdiom == .pad {
                    return UIScreen.main.scale == 2 ? 264 : 132
                }
                if UIScreen.main.scale == 3 {
                    return UIScreen.main.nativeScale == 3 ? 458 : 401
                }
                return 326
        }
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
        uname(&sysinfo) // ignore return value
        return String(bytes: Data(bytes: &sysinfo.machine, count: Int(_SYS_NAMELEN)), encoding: .ascii)!.trimmingCharacters(in: .controlCharacters)
    }()
}
