package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.kmp.feature.map.interop.MapTiled2dMapLayerConfig as SharedLayerConfig
import io.openmobilemaps.mapscore.shared.map.coordinates.Coord as MapscoreCoord
import io.openmobilemaps.mapscore.shared.map.coordinates.CoordinateConversionHelperInterface
import io.openmobilemaps.mapscore.shared.map.layers.tiled.Tiled2dMapLayerConfig as MapscoreLayerConfig
import io.openmobilemaps.mapscore.shared.map.layers.tiled.Tiled2dMapZoomInfo as MapscoreZoomInfo
import io.openmobilemaps.mapscore.shared.map.layers.tiled.Tiled2dMapZoomLevelInfo
import io.openmobilemaps.mapscore.shared.map.layers.tiled.Tiled2dMapVectorSettings
import kotlin.math.pow

class MapTiled2dMapLayerConfigImplementation(
	private val config: SharedLayerConfig,
) : MapscoreLayerConfig() {

	private val coordinateConverter by lazy {
		CoordinateConversionHelperInterface.independentInstance()
	}

	override fun getCoordinateSystemIdentifier(): Int = config.coordinateSystemIdentifier

	override fun getTileUrl(x: Int, y: Int, t: Int, zoom: Int): String {
		val tilesPerAxis = 2.0.pow(zoom.toDouble())

		val bounds = config.bounds
		val wmMinX = bounds.topLeft.x
		val wmMaxX = bounds.bottomRight.x
		val wmMaxY = bounds.topLeft.y
		val wmMinY = bounds.bottomRight.y

		val tileWidth = (wmMaxX - wmMinX) / tilesPerAxis
		val tileHeight = (wmMaxY - wmMinY) / tilesPerAxis

		val wmMinTileX = wmMinX + x * tileWidth
		val wmMaxTileX = wmMinX + (x + 1) * tileWidth
		val wmMaxTileY = wmMaxY - y * tileHeight
		val wmMinTileY = wmMaxY - (y + 1) * tileHeight

		val wmTopLeft = MapscoreCoord(
			systemIdentifier = config.coordinateSystemIdentifier,
			x = wmMinTileX,
			y = wmMaxTileY,
			z = 0.0,
		)
		val wmBottomRight = MapscoreCoord(
			systemIdentifier = config.coordinateSystemIdentifier,
			x = wmMaxTileX,
			y = wmMinTileY,
			z = 0.0,
		)

		val lv95TopLeft = coordinateConverter.convert(config.bboxCoordinateSystemIdentifier, wmTopLeft)
		val lv95BottomRight = coordinateConverter.convert(config.bboxCoordinateSystemIdentifier, wmBottomRight)

		val bbox = listOf(
			lv95TopLeft.x,
			lv95BottomRight.y,
			lv95BottomRight.x,
			lv95TopLeft.y,
		).joinToString(separator = ",") { "%.3f".format(it) }

		val width = config.tileWidth
		val height = config.tileHeight

		return config.urlFormat
			.replace("{bbox}", bbox, ignoreCase = true)
			.replace("{width}", width.toString(), ignoreCase = true)
			.replace("{height}", height.toString(), ignoreCase = true)
	}

	override fun getZoomLevelInfos(): ArrayList<Tiled2dMapZoomLevelInfo> =
		ArrayList<Tiled2dMapZoomLevelInfo>().apply {
			for (zoom in config.minZoomLevel..config.maxZoomLevel) {
				add(createZoomLevelInfo(zoom))
			}
		}

	override fun getVirtualZoomLevelInfos(): ArrayList<Tiled2dMapZoomLevelInfo> =
		ArrayList<Tiled2dMapZoomLevelInfo>().apply {
			if (config.minZoomLevel > 0) {
				for (zoom in 0 until config.minZoomLevel) {
					add(createZoomLevelInfo(zoom))
				}
			}
		}

	override fun getZoomInfo(): MapscoreZoomInfo = config.zoomInfo

	override fun getLayerName(): String = config.layerName

	override fun getVectorSettings(): Tiled2dMapVectorSettings? = null

	override fun getBounds() = config.bounds

	private fun createZoomLevelInfo(zoomLevel: Int): Tiled2dMapZoomLevelInfo {
		val tileCount = 2.0.pow(zoomLevel.toDouble())
		val zoom = config.baseZoom / tileCount
		val width = (config.baseWidth / tileCount).toFloat()
		return Tiled2dMapZoomLevelInfo(
			zoom = zoom,
			tileWidthLayerSystemUnits = width,
			numTilesX = tileCount.toInt(),
			numTilesY = tileCount.toInt(),
			numTilesT = 1,
			zoomLevelIdentifier = zoomLevel,
			bounds = config.bounds,
		)
	}
}
