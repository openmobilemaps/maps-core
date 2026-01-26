package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect abstract class MapCameraInterface {
	abstract fun setBounds(bounds: RectCoord)
	abstract fun moveToCenterPositionZoom(coord: Coord, zoom: Double, animated: Boolean)
	abstract fun setMinZoom(zoom: Double)
	abstract fun setMaxZoom(zoom: Double)
	abstract fun setBoundsRestrictWholeVisibleRect(enabled: Boolean)
}
