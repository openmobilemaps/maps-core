package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.Tiled2dMapVectorLayerInterface as MapscoreVectorLayerInterface
import io.openmobilemaps.mapscore.shared.map.loader.LoaderInterface as MapscoreLoaderInterface

actual open class Tiled2dMapVectorLayerInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapscore(): MapscoreVectorLayerInterface? =
		nativeHandle as? MapscoreVectorLayerInterface

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
			val typedLoaders = ArrayList<MapscoreLoaderInterface>(loaders.size).apply {
				loaders.forEach { add(requireNotNull(it.asMapscore())) }
			}
			val typedFontLoader = fontLoader?.asMapscore() ?: return null
			val typedLocalDataProvider = localDataProvider?.asMapscore()
			val typedZoomInfo = customZoomInfo
			val typedSymbolDelegate = symbolDelegate?.asMapscore()
			val typedSourceUrlParams = sourceUrlParams?.let { HashMap(it) }
			val layer = MapscoreVectorLayerInterface.createExplicitly(
				layerName = layerName,
				styleJson = styleJson,
				localStyleJson = localStyleJson,
				loaders = typedLoaders,
				fontLoader = typedFontLoader,
				localDataProvider = typedLocalDataProvider,
				customZoomInfo = typedZoomInfo,
				symbolDelegate = typedSymbolDelegate,
				sourceUrlParams = typedSourceUrlParams,
			)
			return Tiled2dMapVectorLayerInterface(layer)
		}
	}
}
