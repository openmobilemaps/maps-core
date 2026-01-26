package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect open class MapCameraInterface {
	open fun setBounds(bounds: RectCoord)
	open fun moveToCenterPositionZoom(centerPosition: Coord, zoom: Double, animated: Boolean)
	open fun setMinZoom(minZoom: Double)
	open fun setMaxZoom(maxZoom: Double)
	open fun setBoundsRestrictWholeVisibleRect(enabled: Boolean)
}
