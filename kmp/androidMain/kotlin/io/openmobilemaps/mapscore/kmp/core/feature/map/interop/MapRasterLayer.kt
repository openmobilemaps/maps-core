package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.shared.map.layers.tiled.raster.Tiled2dMapRasterLayerInterface as MapscoreRasterLayerInterface

actual open class MapRasterLayer actual constructor(nativeHandle: Any?) : LayerInterface(
	(nativeHandle as? MapscoreRasterLayerInterface)?.asLayerInterface(),
) {
	private val rasterLayer = nativeHandle as? MapscoreRasterLayerInterface

	actual constructor(layerInterface: Tiled2dMapRasterLayerInterface) : this(layerInterface.asMapscore())

	internal fun rasterLayerInterface(): MapscoreRasterLayerInterface? = rasterLayer
}
