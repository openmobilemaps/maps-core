package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.Tiled2dMapVectorLayerInterface as MapscoreVectorLayer
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.VectorLayerFeatureInfo as MapscoreFeatureInfo
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.VectorLayerFeatureInfoValue as MapscoreFeatureInfoValue
import io.openmobilemaps.mapscore.kmp.feature.map.interop.MapVectorLayerFeatureInfo as SharedFeatureInfo
import io.openmobilemaps.mapscore.kmp.feature.map.interop.MapVectorLayerFeatureInfoValue as SharedFeatureInfoValue

actual class MapVectorLayer actual constructor(nativeHandle: Any?) : LayerInterface(
	(nativeHandle as? MapscoreVectorLayer)?.asLayerInterface(),
) {
	private val layer = nativeHandle as? MapscoreVectorLayer

	actual constructor(layerInterface: Tiled2dMapVectorLayerInterface) : this(layerInterface.asMapscore())

	actual fun setSelectionDelegate(delegate: MapVectorLayerSelectionCallback?) {
		val proxy = delegate?.let { MapVectorLayerSelectionCallbackProxy(it) }
		layer?.setSelectionDelegate(proxy)
	}

	actual fun setGlobalState(state: Map<String, SharedFeatureInfoValue>) {
		val mapped = HashMap<String, MapscoreFeatureInfoValue>()
		state.forEach { (key, value) ->
			mapped[key] = value.asMapscore()
		}
		layer?.setGlobalState(mapped)
	}

	internal fun vectorLayerInterface(): MapscoreVectorLayer? = layer
}

internal fun MapscoreFeatureInfo.asShared(): SharedFeatureInfo {
	val props = HashMap<String, SharedFeatureInfoValue>()
	for ((key, value) in properties) {
		props[key] = value.asShared()
	}
	return SharedFeatureInfo(
		identifier = identifier,
		properties = props,
	)
}

internal fun MapscoreFeatureInfoValue.asShared(): SharedFeatureInfoValue {
	return SharedFeatureInfoValue(
		stringVal = stringVal,
		doubleVal = doubleVal,
		intVal = intVal,
		boolVal = boolVal,
		colorVal = colorVal,
		listFloatVal = listFloatVal,
		listStringVal = listStringVal,
	)
}

private fun SharedFeatureInfoValue.asMapscore(): MapscoreFeatureInfoValue =
	MapscoreFeatureInfoValue(
		stringVal,
		doubleVal,
		intVal,
		boolVal,
		colorVal,
		listFloatVal,
		listStringVal,
	)
