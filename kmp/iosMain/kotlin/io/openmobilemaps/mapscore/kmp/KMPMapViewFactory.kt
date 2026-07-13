package io.openmobilemaps.mapscore.kmp

import platform.UIKit.UIView
import swiftPMImport.io.openmobilemaps.mapscore.kmp.MCMapView

public object KMPMapViewFactory {
	public fun create(mapViewOwner: MapViewOwner): UIView {
		val mapConfig = mapViewOwner.mapConfig
		val is3D = mapConfig.mapCoordinateSystem.identifier == KMCoordinateSystemIdentifiers.UnitSphere()
		val mapView = MCMapView(
			mapConfig = mapConfig.asPlatform(),
			pixelsPerInch = null,
			is3D = is3D,
		)
		mapViewOwner.initialize(mapView.mapInterface.asKmp())
		return mapView
	}
}
