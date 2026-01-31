package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect open class IndexedLayerInterface constructor(nativeHandle: Any? = null) {
    protected val nativeHandle: Any?

    fun getLayerInterface(): LayerInterface
    fun getIndex(): Int
}
