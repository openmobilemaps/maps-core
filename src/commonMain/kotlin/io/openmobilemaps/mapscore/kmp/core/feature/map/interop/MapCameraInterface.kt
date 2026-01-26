package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect abstract class MapCameraInterface constructor(nativeHandle: Any? = null) {
	abstract fun _setBounds(bounds: RectCoord)
	abstract fun _moveToCenterPositionZoom(coord: Coord, zoom: Double, animated: Boolean)
	abstract fun _setMinZoom(zoom: Double)
	abstract fun _setMaxZoom(zoom: Double)
	abstract fun _setBoundsRestrictWholeVisibleRect(enabled: Boolean)
}
