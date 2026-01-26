package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect open class MapCameraInterface {
	open fun setBounds(bounds: RectCoord)
	open fun moveToCenterPositionZoom(coord: Coord, zoom: Double, animated: Boolean)
	open fun setMinZoom(zoom: Double)
	open fun setMaxZoom(zoom: Double)
	open fun setBoundsRestrictWholeVisibleRect(enabled: Boolean)
}
