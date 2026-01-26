package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect abstract class MapInterface constructor(nativeHandle: Any? = null) {
	abstract fun addVectorLayer(layer: MapVectorLayer?)
	abstract fun removeVectorLayer(layer: MapVectorLayer?)
	abstract fun addRasterLayer(layer: MapRasterLayer?)
	abstract fun removeRasterLayer(layer: MapRasterLayer?)
	abstract fun addGpsLayer(layer: MapGpsLayer?)
	abstract fun removeGpsLayer(layer: MapGpsLayer?)
	abstract fun getCamera(): MapCameraInterface?

	companion object {
		fun create(nativeHandle: Any?): MapInterface
	}
}
