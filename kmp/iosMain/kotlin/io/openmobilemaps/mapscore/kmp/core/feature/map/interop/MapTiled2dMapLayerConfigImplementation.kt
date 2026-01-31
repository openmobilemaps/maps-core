package io.openmobilemaps.mapscore.kmp.feature.map.interop

import MapCoreSharedModule.MCCoordinateConversionHelperInterface
import MapCoreSharedModule.MCTiled2dMapLayerConfigProtocol
import MapCoreSharedModule.MCTiled2dMapVectorSettings
import MapCoreSharedModule.MCTiled2dMapZoomInfo
import MapCoreSharedModule.MCTiled2dMapZoomLevelInfo
import kotlin.math.pow
import platform.darwin.NSObject

class MapTiled2dMapLayerConfigImplementation(
	private val config: MapTiled2dMapLayerConfig,
) : NSObject(), MCTiled2dMapLayerConfigProtocol {
	private val coordinateConverter = MCCoordinateConversionHelperInterface.independentInstance()

	override fun getCoordinateSystemIdentifier(): Int = config.coordinateSystemIdentifier

	override fun getTileUrl(x: Int, y: Int, t: Int, zoom: Int): String {
		var url = config.urlFormat
		val tilesPerAxis = 2.0.pow(zoom.toDouble())

		val bounds = config.bounds
		val wmMinX = bounds.topLeft.x
		val wmMaxX = bounds.bottomRight.x
		val wmMaxY = bounds.topLeft.y
		val wmMinY = bounds.bottomRight.y

		val totalWidth = wmMaxX - wmMinX
		val totalHeight = wmMaxY - wmMinY

		val tileWidth = totalWidth / tilesPerAxis
		val tileHeight = totalHeight / tilesPerAxis

		val wmMinTileX = wmMinX + x.toDouble() * tileWidth
		val wmMaxTileX = wmMinX + (x + 1.0) * tileWidth
		val wmMaxTileY = wmMaxY - y.toDouble() * tileHeight
		val wmMinTileY = wmMaxY - (y + 1.0) * tileHeight

		val wmTopLeft = Coord(
			systemIdentifier = config.coordinateSystemIdentifier,
			x = wmMinTileX,
			y = wmMaxTileY,
			z = 0.0,
		)
		val wmBottomRight = Coord(
			systemIdentifier = config.coordinateSystemIdentifier,
			x = wmMaxTileX,
			y = wmMinTileY,
			z = 0.0,
		)

		val targetSystem = config.bboxCoordinateSystemIdentifier
		val topLeft = coordinateConverter?.convert(targetSystem, coordinate = wmTopLeft) ?: wmTopLeft
		val bottomRight = coordinateConverter?.convert(targetSystem, coordinate = wmBottomRight) ?: wmBottomRight

		val bboxString = "${topLeft.x},${bottomRight.y},${bottomRight.x},${topLeft.y}"

		url = url.replace("{bbox}", bboxString)
		url = url.replace("{width}", config.tileWidth.toString())
		url = url.replace("{height}", config.tileHeight.toString())
		url = url.replace("{WIDTH}", config.tileWidth.toString())
		url = url.replace("{HEIGHT}", config.tileHeight.toString())
		return url
	}

	override fun getZoomLevelInfos(): List<MCTiled2dMapZoomLevelInfo> {
		return (config.minZoomLevel..config.maxZoomLevel).map { getZoomLevelInfo(it) }
	}

	override fun getVirtualZoomLevelInfos(): List<MCTiled2dMapZoomLevelInfo> {
		val minZoom = config.minZoomLevel
		if (minZoom <= 0) return emptyList()
		return (0 until minZoom).map { getZoomLevelInfo(it) }
	}

	override fun getZoomInfo(): MCTiled2dMapZoomInfo {
		return config.zoomInfo
	}

	override fun getLayerName(): String = config.layerName

	override fun getVectorSettings(): MCTiled2dMapVectorSettings? = null

	override fun getBounds(): RectCoord? = config.bounds

	private fun getZoomLevelInfo(zoomLevel: Int): MCTiled2dMapZoomLevelInfo {
		val tileCount = 2.0.pow(zoomLevel.toDouble())
		val zoom = config.baseZoom / tileCount
		val width = config.baseWidth / tileCount
		return MCTiled2dMapZoomLevelInfo(
			zoom = zoom,
			tileWidthLayerSystemUnits = width.toFloat(),
			numTilesX = tileCount.toInt(),
			numTilesY = tileCount.toInt(),
			numTilesT = 1,
			zoomLevelIdentifier = zoomLevel,
			bounds = config.bounds,
		)
	}
}
