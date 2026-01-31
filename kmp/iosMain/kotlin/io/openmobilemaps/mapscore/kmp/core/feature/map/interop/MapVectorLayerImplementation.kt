package io.openmobilemaps.mapscore.kmp.feature.map.interop

import MapCoreSharedModule.MCTiled2dMapVectorLayerInterface
import MapCoreSharedModule.MCVectorLayerFeatureInfoValue
import platform.Foundation.NSNumber

actual class MapVectorLayer actual constructor(nativeHandle: Any?) : LayerInterface(
	(nativeHandle as? MCTiled2dMapVectorLayerInterface)?.asLayerInterface(),
) {
	private val layer = nativeHandle as? MCTiled2dMapVectorLayerInterface

	actual fun setSelectionDelegate(delegate: MapVectorLayerSelectionCallback?) {
		val proxy = delegate?.let { MapVectorLayerSelectionCallbackProxy(it) }
		layer?.setSelectionDelegate(proxy)
	}

	actual fun setGlobalState(state: Map<String, MapVectorLayerFeatureInfoValue>) {
		val mapped = mutableMapOf<Any?, Any?>()
		state.forEach { (key, value) ->
			mapped[key] = value.asMapCore()
		}
		layer?.setGlobalState(mapped)
	}

	internal fun vectorLayerInterface(): MCTiled2dMapVectorLayerInterface? = layer
}

private fun MapVectorLayerFeatureInfoValue.asMapCore(): MCVectorLayerFeatureInfoValue {
	val floatList = listFloatVal?.map { NSNumber(float = it) }
	val stringList = listStringVal?.map { it }
	return MCVectorLayerFeatureInfoValue(
		stringVal = stringVal,
		doubleVal = doubleVal?.let { NSNumber(double = it) },
		intVal = intVal?.let { NSNumber(longLong = it) },
		boolVal = boolVal?.let { NSNumber(bool = it) },
		colorVal = colorVal,
		listFloatVal = floatList,
		listStringVal = stringList,
	)
}
