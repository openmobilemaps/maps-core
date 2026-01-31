package io.openmobilemaps.mapscore.kmp.feature.map.interop

import kotlin.experimental.ExperimentalObjCName
import kotlin.native.ObjCName

import MapCoreSharedModule.MCVectorLayerFeatureInfo
import MapCoreSharedModule.MCVectorLayerFeatureInfoValue
import MapCoreSharedModule.MCTiled2dMapVectorLayerSelectionCallbackInterfaceProtocol
import platform.Foundation.NSNumber
import platform.darwin.NSObject

@OptIn(ExperimentalObjCName::class)
@ObjCName("MapVectorLayerSelectionCallbackProxy", exact = true)
actual class MapVectorLayerSelectionCallbackProxy actual constructor(
	handler: MapVectorLayerSelectionCallback,
) : NSObject(), MCTiled2dMapVectorLayerSelectionCallbackInterfaceProtocol {
	actual val handler: MapVectorLayerSelectionCallback = handler
	override fun didSelectFeature(featureInfo: MCVectorLayerFeatureInfo, layerIdentifier: String, coord: Coord): Boolean {
		val sharedFeatureInfo = featureInfo.asShared()
		return handler.didSelectFeature(featureInfo = sharedFeatureInfo, layerIdentifier = layerIdentifier, coord = coord)
	}

	override fun didMultiSelectLayerFeatures(
		featureInfos: List<*>,
		layerIdentifier: String,
		coord: Coord,
	): Boolean {
		val sharedFeatureInfos = featureInfos.mapNotNull { it as? MCVectorLayerFeatureInfo }
			.map { it.asShared() }
		return handler.didMultiSelectLayerFeatures(
			featureInfos = sharedFeatureInfos,
			layerIdentifier = layerIdentifier,
			coord = coord,
		)
	}

	override fun didClickBackgroundConfirmed(coord: Coord): Boolean {
		return handler.didClickBackgroundConfirmed(coord = coord)
	}
}

private fun MCVectorLayerFeatureInfo.asShared(): MapVectorLayerFeatureInfo {
	val props = HashMap<String, MapVectorLayerFeatureInfoValue>()
	for (entry in properties.entries) {
		val key = entry.key as? String ?: continue
		val value = entry.value as? MCVectorLayerFeatureInfoValue ?: continue
		props[key] = value.asShared()
	}
	return MapVectorLayerFeatureInfo(
		identifier = identifier,
		properties = props,
	)
}

private fun MCVectorLayerFeatureInfoValue.asShared(): MapVectorLayerFeatureInfoValue {
	val floatList = listFloatVal
		?.mapNotNull { (it as? NSNumber)?.floatValue }
		?.let { ArrayList(it) }
	val stringList = listStringVal
		?.mapNotNull { it as? String }
		?.let { ArrayList(it) }
	return MapVectorLayerFeatureInfoValue(
		stringVal = stringVal,
		doubleVal = doubleVal?.doubleValue,
		intVal = intVal?.longLongValue,
		boolVal = boolVal?.boolValue,
		colorVal = colorVal,
		listFloatVal = floatList,
		listStringVal = stringList,
	)
}
