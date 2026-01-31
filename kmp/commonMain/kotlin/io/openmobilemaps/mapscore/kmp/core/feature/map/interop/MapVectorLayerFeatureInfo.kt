package io.openmobilemaps.mapscore.kmp.feature.map.interop

data class MapVectorLayerFeatureInfo(
    val identifier: String,
    val properties: HashMap<String, MapVectorLayerFeatureInfoValue>,
)

data class MapVectorLayerFeatureInfoValue(
    val stringVal: String? = null,
    val doubleVal: Double? = null,
    val intVal: Long? = null,
    val boolVal: Boolean? = null,
    val colorVal: Color? = null,
    val listFloatVal: ArrayList<Float>? = null,
    val listStringVal: ArrayList<String>? = null,
)
