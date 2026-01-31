package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.map.layers.TiledRasterLayer

actual open class MapRasterLayer actual constructor(nativeHandle: Any?) : LayerInterface(
	(nativeHandle as? TiledRasterLayer)?.layerInterface(),
) {
	private val rasterLayer = nativeHandle as? TiledRasterLayer

	internal fun rasterLayerInterface(): TiledRasterLayer? = rasterLayer
}
