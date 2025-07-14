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
import SwiftUI

// MARK: - Common Extensions for MCColor

extension MCColor {
    var metalColor: MTLClearColor {
        MTLClearColor(
            red: Double(r),
            green: Double(g),
            blue: Double(b),
            alpha: Double(a)
        )
    }

    /// The SIMD4<Float> representation of this color.
    public var simdValues: SIMD4<Float> {
        SIMD4<Float>(r, g, b, a)
    }
}

// MARK: - UIKit Specific Extensions

#if canImport(UIKit)
    import UIKit

    public extension UIColor {
        /// Converts a UIColor to a MapCore MCColor.
        var mapCoreColor: MCColor {
            var red: CGFloat = 0
            var green: CGFloat = 0
            var blue: CGFloat = 0
            var alpha: CGFloat = 0
            // getRed(_:green:blue:alpha:) returns true if the conversion is successful
            if getRed(&red, green: &green, blue: &blue, alpha: &alpha) {
                return MCColor(r: Float(red), g: Float(green), b: Float(blue), a: Float(alpha))
            }
            // Return a default color if conversion fails
            return MCColor(r: 0, g: 0, b: 0, a: 1.0)
        }

        /// Check if a color is opaque.
        var isOpaque: Bool {
            var alpha: CGFloat = 0
            getRed(nil, green: nil, blue: nil, alpha: &alpha)
            return alpha == 1.0
        }

        /// Check if a color has transparency.
        var hasTransparency: Bool {
            !isOpaque
        }

        /// The Metal clear color reference that corresponds to the receiver’s color.
        var metalClearColor: MTLClearColor {
            var red: CGFloat = 0
            var green: CGFloat = 0
            var blue: CGFloat = 0
            var alpha: CGFloat = 0
            if getRed(&red, green: &green, blue: &blue, alpha: &alpha) {
                return MTLClearColorMake(Double(red), Double(green), Double(blue), Double(alpha))
            }
            return MTLClearColorMake(0, 0, 0, 1.0)
        }
    }

// MARK: - AppKit Specific Extensions
#elseif canImport(AppKit)
    import AppKit

    public extension NSColor {
        /// Converts an NSColor to a MapCore MCColor.
        var mapCoreColor: MCColor {
            // Use the sRGB color space for component extraction to ensure consistency.
            guard let color = usingColorSpace(.sRGB) else {
                // Return a default color if conversion to sRGB is not possible
                return MCColor(r: 0, g: 0, b: 0, a: 1.0)
            }

            var red: CGFloat = 0
            var green: CGFloat = 0
            var blue: CGFloat = 0
            var alpha: CGFloat = 0
            color.getRed(&red, green: &green, blue: &blue, alpha: &alpha)

            return MCColor(r: Float(red), g: Float(green), b: Float(blue), a: Float(alpha))
        }

        /// Check if a color is opaque.
        var isOpaque: Bool {
            alphaComponent == 1.0
        }

        /// Check if a color has transparency.
        var hasTransparency: Bool {
            alphaComponent < 1.0
        }

        /// The Metal clear color reference that corresponds to the receiver’s color.
        var metalClearColor: MTLClearColor {
            guard let color = usingColorSpace(.sRGB) else {
                return MTLClearColorMake(0, 0, 0, 1.0)
            }

            var red: CGFloat = 0
            var green: CGFloat = 0
            var blue: CGFloat = 0
            var alpha: CGFloat = 0
            color.getRed(&red, green: &green, blue: &blue, alpha: &alpha)

            return MTLClearColorMake(Double(red), Double(green), Double(blue), Double(alpha))
        }
    }
#endif
