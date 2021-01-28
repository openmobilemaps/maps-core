import Metal
import UIKit

extension UIColor {
    /// Check if a color is opaque
    var isOpaque: Bool {
        return !hasTransparency
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
