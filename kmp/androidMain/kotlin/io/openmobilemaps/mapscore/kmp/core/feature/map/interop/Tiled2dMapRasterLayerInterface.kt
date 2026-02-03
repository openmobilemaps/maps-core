package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.shared.map.layers.tiled.raster.Tiled2dMapRasterLayerInterface as MapscoreRasterLayerInterface
import io.openmobilemaps.mapscore.shared.map.loader.LoaderInterface as MapscoreLoaderInterface

actual open class Tiled2dMapRasterLayerInterface actual constructor(nativeHandle: Any?) {
    protected actual val nativeHandle: Any? = nativeHandle

    internal fun asMapscore(): MapscoreRasterLayerInterface? =
        nativeHandle as? MapscoreRasterLayerInterface

    actual companion object {
        actual fun create(
            layerConfig: MapTiled2dMapLayerConfig,
            loaders: List<LoaderInterface>,
        ): Tiled2dMapRasterLayerInterface? {
            val typedLoaders = ArrayList<MapscoreLoaderInterface>(loaders.size).apply {
                loaders.forEach { add(requireNotNull(it.asMapscore())) }
            }
            val layer = MapscoreRasterLayerInterface.create(
                MapTiled2dMapLayerConfigImplementation(layerConfig),
                typedLoaders,
            )
            return layer?.let { Tiled2dMapRasterLayerInterface(it) }
        }
    }
}
