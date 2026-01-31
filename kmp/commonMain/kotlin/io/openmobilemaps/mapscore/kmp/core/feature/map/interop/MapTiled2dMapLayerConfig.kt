package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect class Tiled2dMapZoomInfo(
	zoomLevelScaleFactor: Float,
	numDrawPreviousLayers: Int,
	numDrawPreviousOrLaterTLayers: Int,
	adaptScaleToScreen: Boolean,
	maskTile: Boolean,
	underzoom: Boolean,
	overzoom: Boolean,
) {
	val zoomLevelScaleFactor: Float
	val numDrawPreviousLayers: Int
	val numDrawPreviousOrLaterTLayers: Int
	val adaptScaleToScreen: Boolean
	val maskTile: Boolean
	val underzoom: Boolean
	val overzoom: Boolean
}

data class MapTiled2dMapLayerConfig(
	val layerName: String,
	val urlFormat: String,
	val zoomInfo: Tiled2dMapZoomInfo,
	val minZoomLevel: Int,
	val maxZoomLevel: Int,
	val coordinateSystemIdentifier: Int,
	val bboxCoordinateSystemIdentifier: Int,
	val bounds: RectCoord,
	val baseZoom: Double,
	val baseWidth: Double,
	val tileWidth: Int = 512,
	val tileHeight: Int = 512,
)
