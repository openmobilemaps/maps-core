package io.openmobilemaps.mapscore.kmp.feature.map.interop

import kotlin.experimental.ExperimentalObjCName
import kotlin.native.ObjCName

import MapCoreSharedModule.MCVectorLayerFeatureInfo
import MapCoreSharedModule.MCVectorLayerFeatureInfoValue
import MapCoreSharedModule.MCTiled2dMapVectorLayerSelectionCallbackInterfaceProtocol
import platform.darwin.NSObject

@OptIn(ExperimentalObjCName::class)
@ObjCName("MapVectorLayerSelectionCallbackProxy", exact = true)
actual class MapVectorLayerSelectionCallbackProxy actual constructor(
	handler: MapVectorLayerSelectionCallback,
) : NSObject(), MCTiled2dMapVectorLayerSelectionCallbackInterfaceProtocol {
	actual val handler: MapVectorLayerSelectionCallback = handler
	override fun didSelectFeature(featureInfo: MCVectorLayerFeatureInfo, layerIdentifier: String, coord: Coord): Boolean {
		val sharedFeatureInfo = featureInfo.asShared(layerIdentifier)
		return handler._didSelectFeature(featureInfo = sharedFeatureInfo, coord = coord)
	}

	override fun didMultiSelectLayerFeatures(
		featureInfos: List<*>,
		layerIdentifier: String,
		coord: Coord,
	): Boolean {
		return handler._didMultiSelectLayerFeatures(layerIdentifier = layerIdentifier, coord = coord)
	}

	override fun didClickBackgroundConfirmed(coord: Coord): Boolean {
		return handler._didClickBackgroundConfirmed(coord = coord)
	}
}

private fun MCVectorLayerFeatureInfo.asShared(layerIdentifier: String): MapVectorLayerFeatureInfo {
	val props = mutableMapOf<String, MapVectorLayerFeatureInfoValue>()
	for (entry in properties.entries) {
		val key = entry.key as? String ?: continue
		val value = entry.value as? MCVectorLayerFeatureInfoValue ?: continue
		props[key] = value.asShared()
	}
	return MapVectorLayerFeatureInfo(
		identifier = identifier,
		layerIdentifier = layerIdentifier,
		properties = props,
	)
}

private fun MCVectorLayerFeatureInfoValue.asShared(): MapVectorLayerFeatureInfoValue {
	val stringValue = stringVal
		?: intVal?.stringValue
		?: doubleVal?.stringValue
		?: boolVal?.stringValue
	val list = listStringVal?.mapNotNull { it as? String }
	return MapVectorLayerFeatureInfoValue(stringVal = stringValue, listStringVal = list)
}
