package io.openmobilemaps.mapscore.kmp.feature.map.interop

import MapCoreSharedModule.MCTiled2dMapRasterLayerInterface

actual open class Tiled2dMapRasterLayerInterface actual constructor(nativeHandle: Any?) {
    protected actual val nativeHandle: Any? = nativeHandle

    internal fun asMapCore(): MCTiled2dMapRasterLayerInterface? =
        nativeHandle as? MCTiled2dMapRasterLayerInterface

    actual companion object {
        actual fun create(
            layerConfig: MapTiled2dMapLayerConfig,
            loaders: List<LoaderInterface>,
        ): Tiled2dMapRasterLayerInterface? {
            val typedLoaders = loaders.map { requireNotNull(it.asMapCore()) }
            val layer = MCTiled2dMapRasterLayerInterface.create(
                MapTiled2dMapLayerConfigImplementation(layerConfig),
                loaders = typedLoaders,
            )
            return layer?.let { Tiled2dMapRasterLayerInterface(it) }
        }
    }
}
