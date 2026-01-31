package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect class MapVectorLayer constructor(nativeHandle: Any? = null) : LayerInterface {
    fun setSelectionDelegate(delegate: MapVectorLayerSelectionCallback?)
    fun setGlobalState(state: Map<String, MapVectorLayerFeatureInfoValue>)
}
