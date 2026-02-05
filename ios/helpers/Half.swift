//
//  Half.swift
//  MapCore
//
//  Created by Noah Bussinger Ubique on 19.11.2025.
//

///Polyfill Float16 on unsupported Intel platforms
public struct Half: Equatable, Hashable, Sendable {

    /// Raw Float16 bit pattern
    public var bits: UInt16

    public init(bits: UInt16) {
        self.bits = bits
    }

    /// Initialize from Float to Half
    public init(_ value: Float) {
        self.bits = Half.floatToHalfBits(value)
    }

    /// Initialize from Int32 to Float to Half
    public init(_ value: Int32) {
        self.init(Float(value))
    }

    /// Conversion function from Float to Half bits
    private static func floatToHalfBits(_ value: Float) -> UInt16 {
        let f = value.bitPattern
        let sign = UInt16((f >> 31) & 0x1)
        let exp  = Int((f >> 23) & 0xFF) - 127 + 15
        var mant = UInt16((f >> 13) & 0x3FF)

        if exp <= 0 {
            // Subnormal or zero
            if exp < -10 {
                return sign << 15
            }
            mant |= 0x0400
            let shift = UInt16(1 - exp)
            return (sign << 15) | (mant >> shift)
        } else if exp >= 31 {
            // Inf or NaN
            return (sign << 15) | 0x7C00 | (mant > 0 ? 1 : 0)
        }

        return (sign << 15) | (UInt16(exp) << 10) | mant
    }
}
