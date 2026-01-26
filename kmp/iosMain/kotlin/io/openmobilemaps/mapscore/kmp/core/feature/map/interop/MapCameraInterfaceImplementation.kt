package io.openmobilemaps.mapscore.kmp.feature.map.interop

import kotlin.experimental.ExperimentalObjCName
import kotlin.native.ObjCName

import MapCoreSharedModule.MCMapCameraInterface

@OptIn(ExperimentalObjCName::class)
@ObjCName("MapCameraInterface", exact = true)
actual abstract class MapCameraInterface actual constructor(nativeHandle: Any?) {
	protected val nativeHandle: Any? = nativeHandle

	actual abstract fun _setBounds(bounds: RectCoord)
	actual abstract fun _moveToCenterPositionZoom(coord: Coord, zoom: Double, animated: Boolean)
	actual abstract fun _setMinZoom(zoom: Double)
	actual abstract fun _setMaxZoom(zoom: Double)
	actual abstract fun _setBoundsRestrictWholeVisibleRect(enabled: Boolean)
}

internal class MapCameraInterfaceImpl(nativeHandle: Any?) : MapCameraInterface(nativeHandle) {
	private val camera = nativeHandle as? MCMapCameraInterface

	override fun _setBounds(bounds: RectCoord) {
		camera?.setBounds(bounds)
	}

	override fun _moveToCenterPositionZoom(coord: Coord, zoom: Double, animated: Boolean) {
		camera?.moveToCenterPositionZoom(coord, zoom, animated)
	}

	override fun _setMinZoom(zoom: Double) {
		camera?.setMinZoom(zoom)
	}

	override fun _setMaxZoom(zoom: Double) {
		camera?.setMaxZoom(zoom)
	}

	override fun _setBoundsRestrictWholeVisibleRect(enabled: Boolean) {
		camera?.setBoundsRestrictWholeVisibleRect(enabled)
	}
}
