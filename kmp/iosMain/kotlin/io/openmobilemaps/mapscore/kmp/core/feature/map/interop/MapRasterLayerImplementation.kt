package io.openmobilemaps.mapscore.kmp.feature.map.interop

import MapCoreSharedModule.MCTiled2dMapRasterLayerInterface

actual open class MapRasterLayer actual constructor(
	nativeHandle: Any?,
) : LayerInterface(
	(nativeHandle as? MCTiled2dMapRasterLayerInterface)?.asLayerInterface(),
) {
	private val layer = nativeHandle as? MCTiled2dMapRasterLayerInterface

	actual constructor(layerInterface: Tiled2dMapRasterLayerInterface) : this(layerInterface.asMapCore())

	internal fun rasterLayerInterface(): MCTiled2dMapRasterLayerInterface? = layer
}
