package io.openmobilemaps.mapscore.kmp.feature.map.interop

import kotlin.experimental.ExperimentalObjCName
import kotlin.native.ObjCName

import MapCoreSharedModule.MCTiled2dMapRasterLayerInterface

@OptIn(ExperimentalObjCName::class)
@ObjCName("MapRasterLayer", exact = true)
actual open class MapRasterLayer actual constructor(
	nativeHandle: Any?,
) {
	private val layer = nativeHandle as? MCTiled2dMapRasterLayerInterface

	internal fun layerInterface() = layer?.asLayerInterface()
}

internal class MapRasterLayerImpl(nativeHandle: Any?) : MapRasterLayer(nativeHandle)
