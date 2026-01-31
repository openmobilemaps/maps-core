package io.openmobilemaps.mapscore.kmp.feature.map.interop

import MapCoreSharedModule.MCIndexedLayerInterfaceProtocol

actual open class IndexedLayerInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
	private val indexedLayer = nativeHandle as? MCIndexedLayerInterfaceProtocol

	internal fun asMapCore(): MCIndexedLayerInterfaceProtocol? =
		nativeHandle as? MCIndexedLayerInterfaceProtocol

	actual fun getLayerInterface(): LayerInterface =
		LayerInterface(indexedLayer?.getLayerInterface())

	actual fun getIndex(): Int = indexedLayer?.getIndex() ?: 0
}
