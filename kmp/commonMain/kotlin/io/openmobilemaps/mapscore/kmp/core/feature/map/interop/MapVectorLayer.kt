package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect abstract class MapVectorLayer constructor(nativeHandle: Any? = null) {
	abstract fun setSelectionDelegate(delegate: MapVectorLayerSelectionCallbackProxy?)
	abstract fun setGlobalState(state: Map<String, MapVectorLayerFeatureInfoValue>)
}
