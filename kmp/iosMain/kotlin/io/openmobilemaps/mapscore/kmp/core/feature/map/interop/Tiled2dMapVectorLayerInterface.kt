package io.openmobilemaps.mapscore.kmp.feature.map.interop

import MapCoreSharedModule.MCTiled2dMapVectorLayerInterface
import platform.Foundation.NSNumber

actual open class Tiled2dMapVectorLayerInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapCore(): MCTiled2dMapVectorLayerInterface? =
		nativeHandle as? MCTiled2dMapVectorLayerInterface

	actual companion object {
		actual fun createExplicitly(
			layerName: String,
			styleJson: String?,
			localStyleJson: Boolean?,
			loaders: List<LoaderInterface>,
			fontLoader: FontLoaderInterface?,
			localDataProvider: Tiled2dMapVectorLayerLocalDataProviderInterface?,
			customZoomInfo: Tiled2dMapZoomInfo?,
			symbolDelegate: Tiled2dMapVectorLayerSymbolDelegateInterface?,
			sourceUrlParams: Map<String, String>?,
		): Tiled2dMapVectorLayerInterface? {
			val typedLoaders = loaders.map { requireNotNull(it.asMapCore()) }
			val typedFontLoader = fontLoader?.asMapCore()
			val typedLocalDataProvider = localDataProvider?.asMapCore()
			val typedZoomInfo = customZoomInfo
			val typedSymbolDelegate = symbolDelegate?.asMapCore()
			val localStyleJsonNumber = localStyleJson?.let { NSNumber(bool = it) }
			@Suppress("UNCHECKED_CAST")
			val typedSourceUrlParams = sourceUrlParams?.let { it as Map<Any?, *> }
			val layer = MCTiled2dMapVectorLayerInterface.createExplicitly(
				layerName = layerName,
				styleJson = styleJson,
				localStyleJson = localStyleJsonNumber,
				loaders = typedLoaders,
				fontLoader = typedFontLoader,
				localDataProvider = typedLocalDataProvider,
				customZoomInfo = typedZoomInfo,
				symbolDelegate = typedSymbolDelegate,
				sourceUrlParams = typedSourceUrlParams,
			)
			return layer?.let { Tiled2dMapVectorLayerInterface(it) }
		}
	}
}
