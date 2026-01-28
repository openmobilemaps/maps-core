package io.openmobilemaps.mapscore.kmp.feature.map.interop

data class MapVectorLayerFeatureInfo(
    val identifier: String,
    val layerIdentifier: String,
    val properties: Map<String, MapVectorLayerFeatureInfoValue>,
)

data class MapVectorLayerFeatureInfoValue(
    val stringVal: String? = null,
    val listStringVal: List<String>? = null,
)
