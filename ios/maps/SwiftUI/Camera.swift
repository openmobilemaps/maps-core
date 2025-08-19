//
//  Camera.swift
//  MapCore
//
//  Created by Nicolas Märki on 16.08.2025.
//

public struct Camera: Equatable {
    public var position: Coordinate
    public var zoom: Double

    public var minZoom: Double?
    public var maxZoom: Double?
    public var restrictedBounds: MCRectCoord? = nil

    public init(position: Coordinate, zoom: Double, minZoom: Double? = nil, maxZoom: Double? = nil, restrictedBounds: MCRectCoord? = nil) {
        self.position = position
        self.zoom = zoom
        self.minZoom = minZoom
        self.maxZoom = maxZoom
        self.restrictedBounds = restrictedBounds
    }

    public init(latitude: Double, longitude: Double, zoom: Double) {
        self.init(position: Coordinate(latitude: latitude, longitude: longitude), zoom: zoom)
    }

}
