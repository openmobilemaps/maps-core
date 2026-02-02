package io.openmobilemaps.mapscore.kmp.feature.map.interop

import kotlinx.coroutines.CoroutineScope

actual object MapVectorLayerLocalDataProviderFactory {
	actual fun create(
		dataProvider: MapDataProviderProtocol,
		coroutineScope: CoroutineScope?,
	): Tiled2dMapVectorLayerLocalDataProviderInterface {
		val scope = requireNotNull(coroutineScope) {
			"CoroutineScope required for Android vector layer data provider"
		}
		return Tiled2dMapVectorLayerLocalDataProviderInterface(
			MapDataProviderLocalDataProviderImplementation(
				dataProvider = dataProvider,
				coroutineScope = scope,
			),
		)
	}
}
