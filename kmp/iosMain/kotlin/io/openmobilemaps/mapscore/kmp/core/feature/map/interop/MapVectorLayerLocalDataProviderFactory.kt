package io.openmobilemaps.mapscore.kmp.feature.map.interop

import kotlinx.coroutines.CoroutineScope

actual object MapVectorLayerLocalDataProviderFactory {
	actual fun create(
		dataProvider: MapDataProviderProtocol,
		coroutineScope: CoroutineScope?,
	): Tiled2dMapVectorLayerLocalDataProviderInterface {
		return Tiled2dMapVectorLayerLocalDataProviderInterface(
			MapDataProviderLocalDataProviderImplementation(dataProvider),
		)
	}
}
