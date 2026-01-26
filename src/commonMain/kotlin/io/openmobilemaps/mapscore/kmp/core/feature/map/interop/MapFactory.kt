package io.openmobilemaps.mapscore.kmp.feature.map.interop

import kotlinx.coroutines.CoroutineScope

expect abstract class MapFactory constructor(
	platformContext: Any? = null,
	coroutineScope: CoroutineScope? = null,
	lifecycle: Any? = null,
) {
	abstract fun _createVectorLayer(
		layerName: String,
		dataProvider: MapDataProviderProtocol,
	): MapVectorLayer?
	abstract fun _createRasterLayer(config: MapTiled2dMapLayerConfig): MapRasterLayer?
	abstract fun _createGpsLayer(): MapGpsLayer?

	companion object {
		fun create(
			platformContext: Any? = null,
			coroutineScope: CoroutineScope? = null,
			lifecycle: Any? = null,
		): MapFactory
	}
}
