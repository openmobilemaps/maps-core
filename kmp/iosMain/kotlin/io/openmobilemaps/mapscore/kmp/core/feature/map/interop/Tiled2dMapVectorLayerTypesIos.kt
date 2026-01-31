package io.openmobilemaps.mapscore.kmp.feature.map.interop

import MapCoreSharedModule.MCFontLoaderInterfaceProtocol
import MapCoreSharedModule.MCLoaderInterfaceProtocol
import MapCoreSharedModule.MCTiled2dMapVectorLayerLocalDataProviderInterfaceProtocol
import MapCoreSharedModule.MCTiled2dMapVectorLayerSymbolDelegateInterfaceProtocol

actual open class LoaderInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
}

actual open class FontLoaderInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
}

actual open class Tiled2dMapVectorLayerLocalDataProviderInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
}

actual open class Tiled2dMapVectorLayerSymbolDelegateInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
}

internal fun LoaderInterface.asMapCore(): MCLoaderInterfaceProtocol? =
	nativeHandle as? MCLoaderInterfaceProtocol

internal fun FontLoaderInterface.asMapCore(): MCFontLoaderInterfaceProtocol? =
	nativeHandle as? MCFontLoaderInterfaceProtocol

internal fun Tiled2dMapVectorLayerLocalDataProviderInterface.asMapCore(): MCTiled2dMapVectorLayerLocalDataProviderInterfaceProtocol? =
	nativeHandle as? MCTiled2dMapVectorLayerLocalDataProviderInterfaceProtocol

internal fun Tiled2dMapVectorLayerSymbolDelegateInterface.asMapCore(): MCTiled2dMapVectorLayerSymbolDelegateInterfaceProtocol? =
	nativeHandle as? MCTiled2dMapVectorLayerSymbolDelegateInterfaceProtocol
