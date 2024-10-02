//
//  UIImage+Extension.swift
//
//
//  Created by Matthias Felix on 23.08.23.
//

import UIKit

enum UBImageModificationOpaqueMode {
    case asOriginal
    case opaque
    case transparent
}

struct UBColor {
    let r: UInt8
    let g: UInt8
    let b: UInt8
    let a: UInt8
}

extension UIImage {
    func ub_image(with opaqueMode: UBImageModificationOpaqueMode, byApplyingBlockToPixels block: (UBColor, CGPoint) -> UBColor) -> UIImage? {
        guard let cgImage = self.cgImage else { return nil }
        let alpha = cgImage.alphaInfo
        var opaque = alpha == .none || alpha == .noneSkipFirst || alpha == .noneSkipLast

        if opaqueMode == .transparent {
            opaque = false
        } else if opaqueMode == .opaque {
            opaque = true
        }

        UIGraphicsBeginImageContextWithOptions(self.size, opaque, self.scale)

        self.draw(in: CGRect(x: 0, y: 0, width: self.size.width, height: self.size.height), blendMode: .normal, alpha: 1.0)

        guard let ctx = UIGraphicsGetCurrentContext(), let data = ctx.data?.assumingMemoryBound(to: UInt8.self) else {
            UIGraphicsEndImageContext()
            return nil
        }

        for y in 0 ..< Int(self.size.height) {
            for x in 0 ..< Int(self.size.width) {
                let offset = ctx.bytesPerRow * y + 4 * x

                let inColor = UBColor(r: data[offset + 2], g: data[offset + 1], b: data[offset], a: data[offset + 3])

                let outColor = block(inColor, CGPoint(x: x, y: y))

                data[offset] = outColor.b
                data[offset + 1] = outColor.g
                data[offset + 2] = outColor.r
                data[offset + 3] = outColor.a
            }
        }

        let rtimg = UIGraphicsGetImageFromCurrentImageContext()
        UIGraphicsEndImageContext()

        return rtimg
    }

    func ub_metalFixMe() -> UIImage {
        var fixed = false
        let img = self.ub_image(with: .asOriginal) { color, point in
            if !fixed, color.a == 0 {
                fixed = true
                return UBColor(r: 1, g: 1, b: 1, a: 1)
            } else {
                return color
            }
        }

        return img!
    }
}
