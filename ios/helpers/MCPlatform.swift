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
@preconcurrency import MetalKit

#if canImport(UIKit)
    import UIKit

    public typealias MCPlatformImage = UIImage
    public typealias MCPlatformColor = UIColor
    public typealias MCPlatformView = UIView

    @MainActor
    enum MCDisplayMetrics {
        static func nativeScale(for view: MTKView?) -> CGFloat {
            view?.window?.screen.nativeScale ?? UIScreen.main.nativeScale
        }

        static func pixelsPerInch(for view: MTKView?) -> CGFloat {
            _ = view
            return UIScreen.pixelsPerInch
        }
    }
#elseif canImport(AppKit) && !canImport(UIKit)
    import AppKit

    public typealias MCPlatformImage = NSImage
    public typealias MCPlatformColor = NSColor
    public typealias MCPlatformView = NSView

    enum MCDisplayMetrics {
        static func nativeScale(for view: MTKView?) -> CGFloat {
            if Thread.isMainThread {
                return MainActor.assumeIsolated {
                    view?.window?.backingScaleFactor
                        ?? NSScreen.main?.backingScaleFactor
                        ?? 2.0
                }
            }
            return DispatchQueue.main.sync {
                MainActor.assumeIsolated {
                    view?.window?.backingScaleFactor
                        ?? NSScreen.main?.backingScaleFactor
                        ?? 2.0
                }
            }
        }

        static func pixelsPerInch(for view: MTKView?) -> CGFloat {
            let scale = nativeScale(for: view)
            // Typical base macOS display density is ~110 PPI; Retina scales by backing scale.
            return 110.0 * scale
        }
    }

    extension NSImage {
        var mcCgImage: CGImage? {
            var rect = CGRect(origin: .zero, size: size)
            return cgImage(forProposedRect: &rect, context: nil, hints: nil)
        }
    }
#endif

extension CGImage {
    func toImage() -> MCPlatformImage? {
        #if canImport(UIKit)
            let w = Double(width)
            let h = Double(height)
            UIGraphicsBeginImageContext(CGSize(width: w, height: h))
            let context = UIGraphicsGetCurrentContext()
            context?.draw(self, in: CGRect(x: 0, y: 0, width: w, height: h))

            let newImage = UIGraphicsGetImageFromCurrentImageContext()
            UIGraphicsEndImageContext()

            return newImage
        #elseif canImport(AppKit) && !canImport(UIKit)
            return NSImage(cgImage: self, size: NSSize(width: width, height: height))
        #else
            return nil
        #endif
    }
}
