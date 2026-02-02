package io.openmobilemaps.mapscore.kmp.feature.map.interop

import kotlinx.coroutines.CoroutineScope

expect object MapVectorLayerLocalDataProviderFactory {
	fun create(
		dataProvider: MapDataProviderProtocol,
		coroutineScope: CoroutineScope? = null,
	): Tiled2dMapVectorLayerLocalDataProviderInterface
}
