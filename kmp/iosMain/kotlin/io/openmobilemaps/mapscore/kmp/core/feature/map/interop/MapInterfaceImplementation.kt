package io.openmobilemaps.mapscore.kmp.feature.map.interop

import kotlin.experimental.ExperimentalObjCName
import kotlin.native.ObjCName

import MapCoreSharedModule.MCMapInterface

@OptIn(ExperimentalObjCName::class)
@ObjCName("MapInterface", exact = true)
actual abstract class MapInterface actual constructor(nativeHandle: Any?) {
	protected val nativeHandle: Any? = nativeHandle

	actual abstract fun addVectorLayer(layer: MapVectorLayer?)
	actual abstract fun removeVectorLayer(layer: MapVectorLayer?)
	actual abstract fun addRasterLayer(layer: MapRasterLayer?)
	actual abstract fun removeRasterLayer(layer: MapRasterLayer?)
	actual abstract fun addGpsLayer(layer: MapGpsLayer?)
	actual abstract fun removeGpsLayer(layer: MapGpsLayer?)
	actual abstract fun getCamera(): MapCameraInterface?

	actual companion object {
		actual fun create(nativeHandle: Any?): MapInterface = MapInterfaceImpl(nativeHandle)
	}
}

private class MapInterfaceImpl(nativeHandle: Any?) : MapInterface(nativeHandle) {
	private val nativeMapInterface = nativeHandle as? MCMapInterface

	override fun addVectorLayer(layer: MapVectorLayer?) {
		val handle = layer as? MapVectorLayerImpl ?: return
		handle.layerInterface()?.let { nativeMapInterface?.addLayer(it) }
	}

	override fun removeVectorLayer(layer: MapVectorLayer?) {
		val handle = layer as? MapVectorLayerImpl ?: return
		handle.layerInterface()?.let { nativeMapInterface?.removeLayer(it) }
	}

	override fun addRasterLayer(layer: MapRasterLayer?) {
		val handle = layer ?: return
		handle.layerInterface()?.let { nativeMapInterface?.addLayer(it) }
	}

	override fun removeRasterLayer(layer: MapRasterLayer?) {
		val handle = layer ?: return
		handle.layerInterface()?.let { nativeMapInterface?.removeLayer(it) }
	}

	override fun addGpsLayer(layer: MapGpsLayer?) {
		val handle = layer as? MapGpsLayerImpl ?: return
		handle.layerInterface()?.let { nativeMapInterface?.addLayer(it) }
	}

	override fun removeGpsLayer(layer: MapGpsLayer?) {
		val handle = layer as? MapGpsLayerImpl ?: return
		handle.layerInterface()?.let { nativeMapInterface?.removeLayer(it) }
	}

	override fun getCamera(): MapCameraInterface? = nativeMapInterface?.getCamera()
}
