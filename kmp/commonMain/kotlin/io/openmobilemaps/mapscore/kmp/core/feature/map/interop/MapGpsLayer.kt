package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.kmp.feature.map.model.GpsMode

expect abstract class MapGpsLayer constructor(nativeHandle: Any? = null) : LayerInterface {
    abstract fun setMode(mode: GpsMode)
    abstract fun getMode(): GpsMode
    abstract fun setOnModeChangedListener(listener: ((GpsMode) -> Unit)?)
    abstract fun notifyPermissionGranted()
    abstract fun lastLocation(): Coord?

    companion object {
        fun create(platformContext: Any? = null, lifecycle: Any? = null): MapGpsLayer?
    }
}
