package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect class Tiled2dMapVectorLayerInterface constructor(nativeHandle: Any? = null) {
	protected val nativeHandle: Any?

	companion object {
		fun createExplicitly(
			layerName: String,
			styleJson: String?,
			localStyleJson: Boolean?,
			loaders: List<LoaderInterface>,
			fontLoader: FontLoaderInterface?,
			localDataProvider: Tiled2dMapVectorLayerLocalDataProviderInterface?,
			customZoomInfo: Tiled2dMapZoomInfo?,
			symbolDelegate: Tiled2dMapVectorLayerSymbolDelegateInterface?,
			sourceUrlParams: Map<String, String>?,
		): Tiled2dMapVectorLayerInterface?
	}
}
