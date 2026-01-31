package io.openmobilemaps.mapscore.kmp.feature.map.interop

interface MapVectorLayerSelectionCallback {
	fun didSelectFeature(featureInfo: MapVectorLayerFeatureInfo, layerIdentifier: String, coord: Coord): Boolean
	fun didMultiSelectLayerFeatures(
		featureInfos: List<MapVectorLayerFeatureInfo>,
		layerIdentifier: String,
		coord: Coord,
	): Boolean
	fun didClickBackgroundConfirmed(coord: Coord): Boolean
}

expect class MapVectorLayerSelectionCallbackProxy(handler: MapVectorLayerSelectionCallback) {
	val handler: MapVectorLayerSelectionCallback
}
