package io.openmobilemaps.mapscore.kmp.feature.map.interop

import MapCoreSharedModule.MCFontLoaderInterfaceProtocol
import MapCoreSharedModule.MCLoaderInterfaceProtocol
import MapCoreSharedModule.MCTiled2dMapVectorLayerLocalDataProviderInterfaceProtocol
import MapCoreSharedModule.MCTiled2dMapVectorLayerSymbolDelegateInterfaceProtocol

actual open class LoaderInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapCore(): MCLoaderInterfaceProtocol? =
		nativeHandle as? MCLoaderInterfaceProtocol
}

actual open class FontLoaderInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapCore(): MCFontLoaderInterfaceProtocol? =
		nativeHandle as? MCFontLoaderInterfaceProtocol
}

actual open class Tiled2dMapVectorLayerLocalDataProviderInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapCore(): MCTiled2dMapVectorLayerLocalDataProviderInterfaceProtocol? =
		nativeHandle as? MCTiled2dMapVectorLayerLocalDataProviderInterfaceProtocol
}

actual open class Tiled2dMapVectorLayerSymbolDelegateInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapCore(): MCTiled2dMapVectorLayerSymbolDelegateInterfaceProtocol? =
		nativeHandle as? MCTiled2dMapVectorLayerSymbolDelegateInterfaceProtocol
}
