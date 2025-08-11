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
import SwiftUI

#if canImport(UIKit)
    import UIKit
    typealias PlatformColor = UIColor
#elseif canImport(AppKit)
    import AppKit
    typealias PlatformColor = NSColor
#endif

public extension String {
    func toVectorLayerFeatureValue() -> MCVectorLayerFeatureInfoValue {
        return MCVectorLayerFeatureInfoValue(stringVal: self, doubleVal: nil, intVal: nil, boolVal: nil, colorVal: nil, listFloatVal: nil, listStringVal: nil)
    }
}

public extension Bool {
    func toVectorLayerFeatureValue() -> MCVectorLayerFeatureInfoValue {
        return MCVectorLayerFeatureInfoValue(stringVal: nil, doubleVal: nil, intVal: nil, boolVal: self as NSNumber, colorVal: nil, listFloatVal: nil, listStringVal: nil)
    }
}

public extension Double {
    func toVectorLayerFeatureValue() -> MCVectorLayerFeatureInfoValue {
        return MCVectorLayerFeatureInfoValue(stringVal: nil, doubleVal: self as NSNumber, intVal: nil, boolVal: nil, colorVal: nil, listFloatVal: nil, listStringVal: nil)
    }
}

public extension Float {
    func toVectorLayerFeatureValue() -> MCVectorLayerFeatureInfoValue {
        return MCVectorLayerFeatureInfoValue(stringVal: nil, doubleVal: self as NSNumber, intVal: nil, boolVal: nil, colorVal: nil, listFloatVal: nil, listStringVal: nil)
    }
}

public extension Int {
    func toVectorLayerFeatureValue() -> MCVectorLayerFeatureInfoValue {
        return MCVectorLayerFeatureInfoValue(stringVal: nil, doubleVal: nil, intVal: self as NSNumber, boolVal: nil, colorVal: nil, listFloatVal: nil, listStringVal: nil)
    }
}

public extension Int64 {
    func toVectorLayerFeatureValue() -> MCVectorLayerFeatureInfoValue {
        return MCVectorLayerFeatureInfoValue(stringVal: nil, doubleVal: nil, intVal: self as NSNumber, boolVal: nil, colorVal: nil, listFloatVal: nil, listStringVal: nil)
    }
}

public extension Color {
    func toVectorLayerFeatureValue() -> MCVectorLayerFeatureInfoValue {
        return PlatformColor(self).toVectorLayerFeatureValue()
    }
}

#if canImport(UIKit)
    public extension UIColor {
        func toVectorLayerFeatureValue() -> MCVectorLayerFeatureInfoValue {
            return MCVectorLayerFeatureInfoValue(stringVal: nil, doubleVal: nil, intVal: nil, boolVal: nil, colorVal: self.mapCoreColor, listFloatVal: nil, listStringVal: nil)
        }
    }
#elseif canImport(AppKit)
    public extension NSColor {
        func toVectorLayerFeatureValue() -> MCVectorLayerFeatureInfoValue {
            return MCVectorLayerFeatureInfoValue(stringVal: nil, doubleVal: nil, intVal: nil, boolVal: nil, colorVal: self.mapCoreColor, listFloatVal: nil, listStringVal: nil)
        }
    }
#endif

public extension Array where Element == String {
    func toVectorLayerFeatureValue() -> MCVectorLayerFeatureInfoValue {
        return MCVectorLayerFeatureInfoValue(stringVal: nil, doubleVal: nil, intVal: nil, boolVal: nil, colorVal: nil, listFloatVal: nil, listStringVal: self)
    }
}

public extension Array where Element == Float {
    func toVectorLayerFeatureValue() -> MCVectorLayerFeatureInfoValue {
        return MCVectorLayerFeatureInfoValue(stringVal: nil, doubleVal: nil, intVal: nil, boolVal: nil, colorVal: nil, listFloatVal: self.map { $0 as NSNumber }, listStringVal: nil)
    }
}

public extension Array where Element == Double {
    func toVectorLayerFeatureValue() -> MCVectorLayerFeatureInfoValue {
        return MCVectorLayerFeatureInfoValue(stringVal: nil, doubleVal: nil, intVal: nil, boolVal: nil, colorVal: nil, listFloatVal: self.map { $0 as NSNumber }, listStringVal: nil)
    }
}
