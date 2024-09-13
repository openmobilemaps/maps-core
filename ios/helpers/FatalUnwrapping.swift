//
//  FatalUnwrapping.swift
//
//
//  Created by Nicolas Märki on 13.02.2024.
//

import Foundation

infix operator !!: NilCoalescingPrecedence

public func !! <T>(wrapped: T?, failureMessage: @autoclosure () -> Never) -> T {
    guard let unwrapped = wrapped else {
        failureMessage()
    }
    return unwrapped
}

public func !! <T>(wrapped: T?, failureMessage: @autoclosure () -> Error) throws -> T {
    guard let unwrapped = wrapped else {
        throw failureMessage()
    }
    return unwrapped
}
