package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect class MapVectorLayer constructor(nativeHandle: Any? = null) : LayerInterface {
    constructor(layerInterface: Tiled2dMapVectorLayerInterface)
    fun setSelectionDelegate(delegate: MapVectorLayerSelectionCallback?)
    fun setGlobalState(state: Map<String, MapVectorLayerFeatureInfoValue>)
}
