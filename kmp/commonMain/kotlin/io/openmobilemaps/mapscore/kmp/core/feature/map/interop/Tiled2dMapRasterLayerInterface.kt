package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect open class Tiled2dMapRasterLayerInterface constructor(nativeHandle: Any? = null) {
    protected val nativeHandle: Any?

    companion object {
        fun create(
            layerConfig: MapTiled2dMapLayerConfig,
            loaders: List<LoaderInterface>,
        ): Tiled2dMapRasterLayerInterface?
    }
}
