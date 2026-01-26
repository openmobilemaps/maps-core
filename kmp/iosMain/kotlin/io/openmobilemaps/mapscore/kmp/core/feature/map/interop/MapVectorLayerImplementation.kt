package io.openmobilemaps.mapscore.kmp.feature.map.interop

import kotlin.experimental.ExperimentalObjCName
import kotlin.native.ObjCName

import MapCoreSharedModule.MCTiled2dMapVectorLayerInterface
import MapCoreSharedModule.MCVectorLayerFeatureInfoValue

@OptIn(ExperimentalObjCName::class)
@ObjCName("MapVectorLayer", exact = true)
actual abstract class MapVectorLayer actual constructor(nativeHandle: Any?) {
	protected val nativeHandle: Any? = nativeHandle

	actual abstract fun _setSelectionDelegate(delegate: MapVectorLayerSelectionCallbackProxy?)
	actual abstract fun _setGlobalState(state: Map<String, MapVectorLayerFeatureInfoValue>)
}

class MapVectorLayerImpl(nativeHandle: Any?) : MapVectorLayer(nativeHandle) {
	private val layer = nativeHandle as? MCTiled2dMapVectorLayerInterface

	override fun _setSelectionDelegate(delegate: MapVectorLayerSelectionCallbackProxy?) {
		layer?.setSelectionDelegate(delegate)
	}

	override fun _setGlobalState(state: Map<String, MapVectorLayerFeatureInfoValue>) {
		val mapped = mutableMapOf<Any?, Any?>()
		state.forEach { (key, value) ->
			mapped[key] = value.asMapCore()
		}
		layer?.setGlobalState(mapped)
	}

	internal fun layerInterface() = layer?.asLayerInterface()
}

private fun MapVectorLayerFeatureInfoValue.asMapCore() =
    MCVectorLayerFeatureInfoValue(
        stringVal = stringVal,
        doubleVal = null,
        intVal = null,
        boolVal = null,
        colorVal = null,
        listFloatVal = null,
        listStringVal = listStringVal,
    )
