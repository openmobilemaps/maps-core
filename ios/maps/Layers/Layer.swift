//
//  Layer.swift
//
//
//  Created by Nicolas Märki on 14.02.2024.
//

import Foundation
import MapCoreSharedModule

public protocol Layer {
    var interface: MCLayerInterface? { get }
}
