package io.openmobilemaps.mapscore.map.util

import io.openmobilemaps.mapscore.shared.graphics.common.Color
import io.openmobilemaps.mapscore.shared.map.LayerInterface
import io.openmobilemaps.mapscore.shared.map.MapCamera2dInterface
import io.openmobilemaps.mapscore.shared.map.MapInterface

interface MapViewInterface {
	fun setBackgroundColor(color: Color)
	fun addLayer(layer: LayerInterface)
	fun insertLayerAt(layer: LayerInterface, at: Int)
	fun insertLayerAbove(layer: LayerInterface, above: LayerInterface)
	fun insertLayerBelow(layer: LayerInterface, below: LayerInterface)
	fun removeLayer(layer: LayerInterface)
	fun getCamera(): MapCamera2dInterface
	fun requireMapInterface(): MapInterface
}