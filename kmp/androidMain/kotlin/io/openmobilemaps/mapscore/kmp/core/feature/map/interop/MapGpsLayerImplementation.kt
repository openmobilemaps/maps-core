package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.gps.GpsLayer
import io.openmobilemaps.gps.providers.LocationProviderInterface
import io.openmobilemaps.gps.shared.gps.GpsMode as MapscoreGpsMode
import io.openmobilemaps.mapscore.kmp.feature.map.model.GpsMode

actual abstract class MapGpsLayer actual constructor(nativeHandle: Any?) : LayerInterface(
	(nativeHandle as? GpsLayerHandle)?.layer?.asLayerInterface(),
) {
	protected val gpsHandle: GpsLayerHandle? = nativeHandle as? GpsLayerHandle

	actual abstract fun setMode(mode: GpsMode)
	actual abstract fun getMode(): GpsMode
	actual abstract fun setOnModeChangedListener(listener: ((GpsMode) -> Unit)?)
	actual abstract fun notifyPermissionGranted()
	actual abstract fun lastLocation(): Coord?
}

class MapGpsLayerImpl(nativeHandle: Any?) : MapGpsLayer(nativeHandle) {
	private val handle = gpsHandle
	private val gpsLayer = handle?.layer
	private val locationProvider = handle?.locationProvider
	private var modeListener: ((GpsMode) -> Unit)? = null

	init {
		gpsLayer?.setOnModeChangedListener { mode ->
			modeListener?.invoke(mode.asShared())
		}
	}

	override fun setMode(mode: GpsMode) {
		gpsLayer?.setMode(mode.asMapscore())
	}

	override fun getMode(): GpsMode = gpsLayer?.layerInterface?.getMode()?.asShared() ?: GpsMode.DISABLED

	override fun setOnModeChangedListener(listener: ((GpsMode) -> Unit)?) {
		modeListener = listener
	}

	override fun notifyPermissionGranted() {
		locationProvider?.notifyLocationPermissionGranted()
	}

	override fun lastLocation(): Coord? = locationProvider?.getLastLocation()
}

private fun GpsMode.asMapscore(): MapscoreGpsMode = when (this) {
	GpsMode.DISABLED -> MapscoreGpsMode.DISABLED
	GpsMode.STANDARD -> MapscoreGpsMode.STANDARD
	GpsMode.FOLLOW -> MapscoreGpsMode.FOLLOW
}

private fun MapscoreGpsMode.asShared(): GpsMode = when (this) {
	MapscoreGpsMode.DISABLED -> GpsMode.DISABLED
	MapscoreGpsMode.STANDARD -> GpsMode.STANDARD
	MapscoreGpsMode.FOLLOW -> GpsMode.FOLLOW
	MapscoreGpsMode.FOLLOW_AND_TURN -> GpsMode.FOLLOW
}

data class GpsLayerHandle(
	val layer: GpsLayer,
	val locationProvider: LocationProviderInterface,
)
