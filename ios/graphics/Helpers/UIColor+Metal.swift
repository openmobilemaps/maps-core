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

extension UIColor {
    /// Check if a color is opaque
    var isOpaque: Bool {
        !hasTransparency
    }

    /// Check if a color has transparency
    var hasTransparency: Bool {
        let color = cgColor
        guard let components = color.components, let alpha = components.last else {
            return false
        }
        return alpha < 1.0
    }

    /// The Metal clear color reference that corresponds to the receiverâ€™s color.
    var metalClearColor: MTLClearColor {
        let color = cgColor
        guard let components = color.components, let alpha = components.last else {
            return MTLClearColorMake(0, 0, 0, 1.0)
        }
        let r, g, b: CGFloat
        switch color.numberOfComponents {
        case 2:
            r = components[0]
            g = components[0]
            b = components[0]
        case 4:
            r = components[0]
            g = components[1]
            b = components[2]
        default:
            return MTLClearColorMake(0, 0, 0, Double(alpha))
        }
        return MTLClearColorMake(Double(r), Double(g), Double(b), Double(alpha))
    }
}

extension MCColor {
    var metalColor: MTLClearColor {
        MTLClearColor(red: Double(r),
                      green: Double(g),
                      blue: Double(b),
                      alpha: Double(a))
    }

    var simdValues: SIMD4<Float> {
        SIMD4<Float>(r, g, b, a)
    }
}

public extension UIColor {
    var mapCoreColor: MCColor {
        let color = cgColor
        guard let components = color.components, let alpha = components.last else {
            return MCColor(r: 0, g: 0, b: 0, a: 1.0)
        }
        let r, g, b: CGFloat
        switch color.numberOfComponents {
        case 2:
            r = components[0]
            g = components[0]
            b = components[0]
        case 4:
            r = components[0]
            g = components[1]
            b = components[2]
        default:
            return MCColor(r: 0, g: 0, b: 0, a: Float(alpha))
        }
        return MCColor(r: Float(r), g: Float(g), b: Float(b), a: Float(alpha))
    }
}
