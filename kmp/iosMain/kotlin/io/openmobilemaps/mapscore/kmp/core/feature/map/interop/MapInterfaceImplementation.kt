package io.openmobilemaps.mapscore.kmp.feature.map.interop

import kotlin.experimental.ExperimentalObjCName
import kotlin.native.ObjCName

import MapCoreSharedModule.MCMapInterface

@OptIn(ExperimentalObjCName::class)
@ObjCName("MapInterface", exact = true)
actual abstract class MapInterface actual constructor(nativeHandle: Any?) {
	protected val nativeHandle: Any? = nativeHandle

	actual abstract fun _addVectorLayer(layer: MapVectorLayer?)
	actual abstract fun _removeVectorLayer(layer: MapVectorLayer?)
	actual abstract fun _addRasterLayer(layer: MapRasterLayer?)
	actual abstract fun _removeRasterLayer(layer: MapRasterLayer?)
	actual abstract fun _addGpsLayer(layer: MapGpsLayer?)
	actual abstract fun _removeGpsLayer(layer: MapGpsLayer?)
	actual abstract fun _getCamera(): MapCameraInterface?

	actual companion object {
		actual fun create(nativeHandle: Any?): MapInterface = MapInterfaceImpl(nativeHandle)
	}
}

private class MapInterfaceImpl(nativeHandle: Any?) : MapInterface(nativeHandle) {
	private val nativeMapInterface = nativeHandle as? MCMapInterface
	private val cameraInterface = MapCameraInterfaceImpl(nativeMapInterface?.getCamera())

	override fun _addVectorLayer(layer: MapVectorLayer?) {
		val handle = layer as? MapVectorLayerImpl ?: return
		handle.layerInterface()?.let { nativeMapInterface?.addLayer(it) }
	}

	override fun _removeVectorLayer(layer: MapVectorLayer?) {
		val handle = layer as? MapVectorLayerImpl ?: return
		handle.layerInterface()?.let { nativeMapInterface?.removeLayer(it) }
	}

	override fun _addRasterLayer(layer: MapRasterLayer?) {
		val handle = layer ?: return
		handle.layerInterface()?.let { nativeMapInterface?.addLayer(it) }
	}

	override fun _removeRasterLayer(layer: MapRasterLayer?) {
		val handle = layer ?: return
		handle.layerInterface()?.let { nativeMapInterface?.removeLayer(it) }
	}

	override fun _addGpsLayer(layer: MapGpsLayer?) {
		val handle = layer as? MapGpsLayerImpl ?: return
		handle.layerInterface()?.let { nativeMapInterface?.addLayer(it) }
	}

	override fun _removeGpsLayer(layer: MapGpsLayer?) {
		val handle = layer as? MapGpsLayerImpl ?: return
		handle.layerInterface()?.let { nativeMapInterface?.removeLayer(it) }
	}

	override fun _getCamera(): MapCameraInterface? = cameraInterface
}
