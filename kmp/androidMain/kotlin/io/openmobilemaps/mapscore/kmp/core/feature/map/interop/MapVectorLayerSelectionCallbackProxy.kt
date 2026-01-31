package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.Tiled2dMapVectorLayerSelectionCallbackInterface as MapscoreSelectionCallback
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.VectorLayerFeatureInfo as MapscoreFeatureInfo

actual class MapVectorLayerSelectionCallbackProxy actual constructor(
	handler: MapVectorLayerSelectionCallback,
) : MapscoreSelectionCallback() {
	actual val handler: MapVectorLayerSelectionCallback = handler

	override fun didSelectFeature(
		featureInfo: MapscoreFeatureInfo,
		layerIdentifier: String,
		coord: Coord,
	): Boolean {
		val shared = featureInfo.asShared()
		return handler.didSelectFeature(shared, layerIdentifier, coord)
	}

	override fun didMultiSelectLayerFeatures(
		featureInfos: ArrayList<MapscoreFeatureInfo>,
		layerIdentifier: String,
		coord: Coord,
	): Boolean {
		val sharedInfos = featureInfos.map { it.asShared() }
		return handler.didMultiSelectLayerFeatures(sharedInfos, layerIdentifier, coord)
	}

	override fun didClickBackgroundConfirmed(coord: Coord): Boolean {
		return handler.didClickBackgroundConfirmed(coord)
	}
}
