package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.shared.map.IndexedLayerInterface as MapscoreIndexedLayerInterface

actual open class IndexedLayerInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
	private val indexedLayer = nativeHandle as? MapscoreIndexedLayerInterface

	internal fun asMapscore(): MapscoreIndexedLayerInterface? =
		nativeHandle as? MapscoreIndexedLayerInterface

	actual fun getLayerInterface(): LayerInterface =
		LayerInterface(indexedLayer?.getLayerInterface())

	actual fun getIndex(): Int = indexedLayer?.getIndex() ?: 0
}
