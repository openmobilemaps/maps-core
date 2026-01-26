package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.kmp.feature.map.model.GpsMode

expect abstract class MapGpsLayer constructor(nativeHandle: Any? = null) {
	abstract fun _setMode(mode: GpsMode)
	abstract fun _getMode(): GpsMode
	abstract fun _setOnModeChangedListener(listener: ((GpsMode) -> Unit)?)
	abstract fun _notifyPermissionGranted()
	abstract fun _lastLocation(): Coord?
}
