package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect abstract class MapInterface constructor(nativeHandle: Any? = null) {
	abstract fun _addVectorLayer(layer: MapVectorLayer?)
	abstract fun _removeVectorLayer(layer: MapVectorLayer?)
	abstract fun _addRasterLayer(layer: MapRasterLayer?)
	abstract fun _removeRasterLayer(layer: MapRasterLayer?)
	abstract fun _addGpsLayer(layer: MapGpsLayer?)
	abstract fun _removeGpsLayer(layer: MapGpsLayer?)
	abstract fun _getCamera(): MapCameraInterface?

	companion object {
		fun create(nativeHandle: Any?): MapInterface
	}
}
