package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.kmp.feature.map.model.GpsMode
import io.openmobilemaps.gps.GpsLayer
import io.openmobilemaps.gps.providers.LocationProviderInterface
import io.openmobilemaps.gps.shared.gps.GpsMode as MapscoreGpsMode
import io.openmobilemaps.mapscore.map.layers.TiledRasterLayer
import io.openmobilemaps.mapscore.map.view.MapView as MapscoreMapView
import io.openmobilemaps.mapscore.shared.map.LayerInterface
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.Tiled2dMapVectorLayerInterface as MapscoreVectorLayer
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.Tiled2dMapVectorLayerSelectionCallbackInterface as MapscoreSelectionCallback
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.VectorLayerFeatureInfo as MapscoreFeatureInfo
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.VectorLayerFeatureInfoValue as MapscoreFeatureInfoValue
import io.openmobilemaps.mapscore.kmp.feature.map.interop.MapVectorLayerFeatureInfo as SharedFeatureInfo
import io.openmobilemaps.mapscore.kmp.feature.map.interop.MapVectorLayerFeatureInfoValue as SharedFeatureInfoValue

actual abstract class MapInterface actual constructor(nativeHandle: Any?) {
	protected val nativeHandle: Any? = nativeHandle

	actual abstract fun _addVectorLayer(layer: MapVectorLayer?)
	actual abstract fun _removeVectorLayer(layer: MapVectorLayer?)
	actual abstract fun _addRasterLayer(layer: MapRasterLayer?)
	actual abstract fun _removeRasterLayer(layer: MapRasterLayer?)
	actual abstract fun _addGpsLayer(layer: MapGpsLayer?)
	actual abstract fun _removeGpsLayer(layer: MapGpsLayer?)
	actual abstract fun _getCamera(): MapCameraInterface?

	actual companion object {
		actual fun create(nativeHandle: Any?): MapInterface = MapInterfaceImpl(nativeHandle)
	}
}

private class MapInterfaceImpl(nativeHandle: Any?) : MapInterface(nativeHandle) {
	private val mapView = nativeHandle as? MapscoreMapView
	private val cameraInterface = MapCameraInterfaceImpl(mapView?.getCamera())

	override fun _addVectorLayer(layer: MapVectorLayer?) {
		val handle = layer as? MapVectorLayerImpl ?: return
		handle.layerInterface()?.let { mapView?.addLayer(it) }
	}

	override fun _removeVectorLayer(layer: MapVectorLayer?) {
		val handle = layer as? MapVectorLayerImpl ?: return
		handle.layerInterface()?.let { mapView?.removeLayer(it) }
	}

	override fun _addRasterLayer(layer: MapRasterLayer?) {
		val handle = layer as? MapRasterLayer ?: return
		handle.layerInterface()?.let { mapView?.addLayer(it) }
	}

	override fun _removeRasterLayer(layer: MapRasterLayer?) {
		val handle = layer as? MapRasterLayer ?: return
		handle.layerInterface()?.let { mapView?.removeLayer(it) }
	}

	override fun _addGpsLayer(layer: MapGpsLayer?) {
		val handle = layer as? MapGpsLayerImpl ?: return
		handle.layerInterface()?.let { mapView?.addLayer(it) }
	}

	override fun _removeGpsLayer(layer: MapGpsLayer?) {
		val handle = layer as? MapGpsLayerImpl ?: return
		handle.layerInterface()?.let { mapView?.removeLayer(it) }
	}

	override fun _getCamera(): MapCameraInterface? = cameraInterface
}

actual abstract class MapCameraInterface actual constructor(nativeHandle: Any?) {
	protected val nativeHandle: Any? = nativeHandle

	actual abstract fun _setBounds(bounds: RectCoord)
	actual abstract fun _moveToCenterPositionZoom(coord: Coord, zoom: Double, animated: Boolean)
	actual abstract fun _setMinZoom(zoom: Double)
	actual abstract fun _setMaxZoom(zoom: Double)
	actual abstract fun _setBoundsRestrictWholeVisibleRect(enabled: Boolean)
}

private class MapCameraInterfaceImpl(nativeHandle: Any?) : MapCameraInterface(nativeHandle) {
	private val camera = nativeHandle as? io.openmobilemaps.mapscore.map.camera.MapCamera

	override fun _setBounds(bounds: RectCoord) {
		camera?.setBounds(bounds)
	}

	override fun _moveToCenterPositionZoom(coord: Coord, zoom: Double, animated: Boolean) {
		camera?.moveToCenterPositionZoom(coord, zoom, animated)
	}

	override fun _setMinZoom(zoom: Double) {
		camera?.setMinZoom(zoom)
	}

	override fun _setMaxZoom(zoom: Double) {
		camera?.setMaxZoom(zoom)
	}

	override fun _setBoundsRestrictWholeVisibleRect(enabled: Boolean) {
		camera?.setBoundsRestrictWholeVisibleRect(enabled)
	}
}

actual abstract class MapVectorLayer actual constructor(nativeHandle: Any?) {
	protected val nativeHandle: Any? = nativeHandle

	actual abstract fun _setSelectionDelegate(delegate: MapVectorLayerSelectionCallbackProxy?)
	actual abstract fun _setGlobalState(state: Map<String, SharedFeatureInfoValue>)
}

internal class MapVectorLayerImpl(nativeHandle: Any?) : MapVectorLayer(nativeHandle) {
	private val layer = nativeHandle as? MapscoreVectorLayer

	override fun _setSelectionDelegate(delegate: MapVectorLayerSelectionCallbackProxy?) {
		val callback = delegate?.let { MapVectorLayerSelectionCallbackAdapterImplementation(it) }
		layer?.setSelectionDelegate(callback)
	}

	override fun _setGlobalState(state: Map<String, SharedFeatureInfoValue>) {
		val mapped = HashMap<String, MapscoreFeatureInfoValue>()
		state.forEach { (key, value) ->
			mapped[key] = value.asMapscore()
		}
		layer?.setGlobalState(mapped)
	}

	internal fun layerInterface(): LayerInterface? = layer?.asLayerInterface()
}

actual open class MapRasterLayer actual constructor(nativeHandle: Any?) {
	protected val nativeHandle: Any? = nativeHandle

	internal fun layerInterface(): LayerInterface? =
		(nativeHandle as? TiledRasterLayer)?.layerInterface()
}

actual abstract class MapGpsLayer actual constructor(nativeHandle: Any?) {
	protected val nativeHandle: Any? = nativeHandle

	actual abstract fun _setMode(mode: GpsMode)
	actual abstract fun _getMode(): GpsMode
	actual abstract fun _setOnModeChangedListener(listener: ((GpsMode) -> Unit)?)
	actual abstract fun _notifyPermissionGranted()
	actual abstract fun _lastLocation(): Coord?
}

internal class MapGpsLayerImpl(nativeHandle: Any?) : MapGpsLayer(nativeHandle) {
	private val handle = nativeHandle as? GpsLayerHandle
	private val gpsLayer = handle?.layer
	private val locationProvider = handle?.locationProvider
	private var modeListener: ((GpsMode) -> Unit)? = null

	init {
		gpsLayer?.setOnModeChangedListener { mode ->
			modeListener?.invoke(mode.asShared())
		}
	}

	override fun _setMode(mode: GpsMode) {
		gpsLayer?.setMode(mode.asMapscore())
	}

	override fun _getMode(): GpsMode = gpsLayer?.layerInterface?.getMode()?.asShared() ?: GpsMode.DISABLED

	override fun _setOnModeChangedListener(listener: ((GpsMode) -> Unit)?) {
		modeListener = listener
	}

	override fun _notifyPermissionGranted() {
		locationProvider?.notifyLocationPermissionGranted()
	}

	override fun _lastLocation(): Coord? = locationProvider?.getLastLocation()

	internal fun layerInterface(): LayerInterface? = gpsLayer?.asLayerInterface()
}

private class MapVectorLayerSelectionCallbackAdapterImplementation(
	private val proxy: MapVectorLayerSelectionCallbackProxy,
) : MapscoreSelectionCallback() {
	override fun didSelectFeature(featureInfo: MapscoreFeatureInfo, layerIdentifier: String, coord: Coord): Boolean {
		val shared = featureInfo.asShared(layerIdentifier)
		return proxy.handler._didSelectFeature(shared, coord)
	}

	override fun didMultiSelectLayerFeatures(
		featureInfos: ArrayList<MapscoreFeatureInfo>,
		layerIdentifier: String,
		coord: Coord,
	): Boolean {
		return proxy.handler._didMultiSelectLayerFeatures(layerIdentifier, coord)
	}

	override fun didClickBackgroundConfirmed(coord: Coord): Boolean {
		return proxy.handler._didClickBackgroundConfirmed(coord)
	}
}

private fun MapscoreFeatureInfo.asShared(layerIdentifier: String): SharedFeatureInfo {
	val props = properties.mapValues { it.value.asShared() }
	return SharedFeatureInfo(
		identifier = identifier,
		layerIdentifier = layerIdentifier,
		properties = props,
	)
}

private fun MapscoreFeatureInfoValue.asShared(): SharedFeatureInfoValue {
	val stringValue = stringVal
		?: intVal?.toString()
		?: doubleVal?.toString()
		?: boolVal?.toString()
	val list = listStringVal?.filterIsInstance<String>()
	return SharedFeatureInfoValue(stringVal = stringValue, listStringVal = list)
}

private fun SharedFeatureInfoValue.asMapscore(): MapscoreFeatureInfoValue =
	MapscoreFeatureInfoValue(
		stringVal,
		null,
		null,
		null,
		null,
		null,
		listStringVal?.let { ArrayList(it) },
	)

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

internal data class GpsLayerHandle(
	val layer: GpsLayer,
	val locationProvider: LocationProviderInterface,
)
