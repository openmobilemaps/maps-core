// Those are public API functions and hence disable not used warning
@file:Suppress("unused")

package io.openmobilemaps.mapscore.extensions

import io.openmobilemaps.mapscore.shared.graphics.common.Color
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.VectorLayerFeatureInfoValue

fun String.toVectorLayerFeatureValue() = VectorLayerFeatureInfoValue(stringVal = this, boolVal = null, doubleVal = null, intVal = null, colorVal = null, listStringVal = null, listFloatVal = null)
fun Boolean.toVectorLayerFeatureValue() = VectorLayerFeatureInfoValue(stringVal = null, boolVal = this, doubleVal = null, intVal = null, colorVal = null, listStringVal = null, listFloatVal = null)
fun Double.toVectorLayerFeatureValue() = VectorLayerFeatureInfoValue(stringVal = null, boolVal = null, doubleVal = this, intVal = null, colorVal = null, listStringVal = null, listFloatVal = null)
fun Float.toVectorLayerFeatureValue() = VectorLayerFeatureInfoValue(stringVal = null, boolVal = null, doubleVal = this.toDouble(), intVal = null, colorVal = null, listStringVal = null, listFloatVal = null)
fun Int.toVectorLayerFeatureValue() = VectorLayerFeatureInfoValue(stringVal = null, boolVal = null, doubleVal = null, intVal = this.toLong(), colorVal = null, listStringVal = null, listFloatVal = null)
fun Long.toVectorLayerFeatureValue() = VectorLayerFeatureInfoValue(stringVal = null, boolVal = null, doubleVal = null, intVal = this, colorVal = null, listStringVal = null, listFloatVal = null)
fun Color.toVectorLayerFeatureValue() = VectorLayerFeatureInfoValue(stringVal = null, boolVal = null, doubleVal = null, intVal = null, colorVal = this, listStringVal = null, listFloatVal = null)
fun List<String>.toVectorLayerFeatureValue() = VectorLayerFeatureInfoValue(stringVal = null, boolVal = null, doubleVal = null, intVal = null, colorVal = null, listStringVal = ArrayList(this), listFloatVal = null)
fun List<Float>.toVectorLayerFeatureValue() = VectorLayerFeatureInfoValue(stringVal = null, boolVal = null, doubleVal = null, intVal = null, colorVal = null, listStringVal = null, listFloatVal = ArrayList(this))
fun List<Double>.toVectorLayerFeatureValue() = VectorLayerFeatureInfoValue(stringVal = null, boolVal = null, doubleVal = null, intVal = null, colorVal = null, listStringVal = null, listFloatVal = ArrayList(this.map { it.toFloat() }))