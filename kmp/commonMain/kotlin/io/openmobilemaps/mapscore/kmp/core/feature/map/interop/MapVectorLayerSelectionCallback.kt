package io.openmobilemaps.mapscore.kmp.feature.map.interop

interface MapVectorLayerSelectionCallback {
	fun _didSelectFeature(featureInfo: MapVectorLayerFeatureInfo, coord: Coord): Boolean
	fun _didMultiSelectLayerFeatures(layerIdentifier: String, coord: Coord): Boolean
	fun _didClickBackgroundConfirmed(coord: Coord): Boolean
}

expect class MapVectorLayerSelectionCallbackProxy(handler: MapVectorLayerSelectionCallback) {
	val handler: MapVectorLayerSelectionCallback
}
