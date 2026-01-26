package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect abstract class MapVectorLayer constructor(nativeHandle: Any? = null) {
	abstract fun _setSelectionDelegate(delegate: MapVectorLayerSelectionCallbackProxy?)
	abstract fun _setGlobalState(state: Map<String, MapVectorLayerFeatureInfoValue>)
}
