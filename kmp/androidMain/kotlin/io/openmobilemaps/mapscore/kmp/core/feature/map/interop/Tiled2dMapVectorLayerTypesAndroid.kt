package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.Tiled2dMapVectorLayerLocalDataProviderInterface as MapscoreLocalDataProvider
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.Tiled2dMapVectorLayerSymbolDelegateInterface as MapscoreSymbolDelegate
import io.openmobilemaps.mapscore.shared.map.loader.FontLoaderInterface as MapscoreFontLoader
import io.openmobilemaps.mapscore.shared.map.loader.LoaderInterface as MapscoreLoader

actual open class LoaderInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapscore(): MapscoreLoader? =
		nativeHandle as? MapscoreLoader
}

actual open class FontLoaderInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapscore(): MapscoreFontLoader? =
		nativeHandle as? MapscoreFontLoader
}

actual open class Tiled2dMapVectorLayerLocalDataProviderInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapscore(): MapscoreLocalDataProvider? =
		nativeHandle as? MapscoreLocalDataProvider
}

actual open class Tiled2dMapVectorLayerSymbolDelegateInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapscore(): MapscoreSymbolDelegate? =
		nativeHandle as? MapscoreSymbolDelegate
}
