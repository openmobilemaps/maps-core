package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.map.layers.TiledRasterLayer
import io.openmobilemaps.mapscore.shared.map.LayerInterface

actual open class MapRasterLayer actual constructor(nativeHandle: Any?) {
	protected val nativeHandle: Any? = nativeHandle

	internal fun layerInterface(): LayerInterface? =
		(nativeHandle as? TiledRasterLayer)?.layerInterface()
}
