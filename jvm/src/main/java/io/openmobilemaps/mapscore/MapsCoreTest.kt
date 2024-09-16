package io.openmobilemaps.mapscore

import io.openmobilemaps.mapscore.MapsCore;
import io.openmobilemaps.mapscore.graphics.util.OSMesa
import io.openmobilemaps.mapscore.map.loader.DataLoader;
import io.openmobilemaps.mapscore.map.loader.FontLoader;
import io.openmobilemaps.mapscore.shared.graphics.common.Color
import io.openmobilemaps.mapscore.shared.graphics.common.Vec2I
import io.openmobilemaps.mapscore.shared.map.MapCallbackInterface
import io.openmobilemaps.mapscore.shared.map.MapCamera2dInterface
import io.openmobilemaps.mapscore.shared.map.MapConfig
import io.openmobilemaps.mapscore.shared.map.coordinates.Coord
import io.openmobilemaps.mapscore.shared.map.coordinates.RectCoord
import io.openmobilemaps.mapscore.shared.map.coordinates.CoordinateSystemIdentifiers
import io.openmobilemaps.mapscore.shared.map.MapInterface
import io.openmobilemaps.mapscore.shared.map.LayerReadyState
import io.openmobilemaps.mapscore.shared.map.MapReadyCallbackInterface
import io.openmobilemaps.mapscore.shared.map.coordinates.CoordinateSystemFactory
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.Tiled2dMapVectorLayerInterface
import io.openmobilemaps.mapscore.shared.map.loader.Font
import io.openmobilemaps.mapscore.shared.map.loader.FontLoaderInterface
import io.openmobilemaps.mapscore.shared.map.loader.FontLoaderResult
import io.openmobilemaps.mapscore.shared.map.loader.LoaderStatus
import io.openmobilemaps.mapscore.shared.map.layers.tiled.Tiled2dMapZoomLevelInfo;
import io.openmobilemaps.mapscore.shared.map.layers.tiled.DefaultTiled2dMapLayerConfigs;

import java.io.File
import javax.imageio.ImageIO

class MyMapReadyCallbackInterface : MapReadyCallbackInterface() {
  override fun stateDidUpdate(state: LayerReadyState) {
    println(state)
  }
}
/*
class MyErrorManager : io.openmobilemaps.mapscore.shared.map.ErrorManager() {
    abstract fun addTiledLayerError(error: TiledLayerError)
    abstract fun removeError(url: String)
    abstract fun removeAllErrorsForLayer(layerName: String)
    abstract fun clearAllErrors()
    abstract fun addErrorListener(listener: ErrorManagerListener)
    abstract fun removeErrorListener(listener: ErrorManagerListener)
}*/

fun main() {
  MapsCore.initialize()
  var dpi = 90.0;

  val config = MapConfig(CoordinateSystemFactory.getEpsg3857System())

  val mapInterface = MapInterface.createWithOpenGl(config, dpi.toFloat())
  mapInterface.setCallbackHandler(
          object : MapCallbackInterface() {
            override fun invalidate() {}
            override fun onMapResumed() {}
          }
  )

  var ctx = OSMesa(900, 550)
  mapInterface.getRenderingContext().onSurfaceCreated()
  mapInterface.setViewportSize(Vec2I(900, 550))

  //mapInterface.setBackgroundColor(Color(0.9f, 0.9f, 1.0f, 1.0f))
  mapInterface.setBackgroundColor(Color(0.0f, 0.0f, 0.0f, 0.0f))
  var cam = mapInterface.getCamera()
  

  var wmts = DefaultTiled2dMapLayerConfigs.webMercator("nöthing", "gär nünt");
  val zooms = wmts.getZoomLevelInfos();

  cam.moveToCenterPositionZoom(Coord(CoordinateSystemIdentifiers.EPSG4326() , 8.2, 46.8, 0.0), zooms[8].zoom, false);// 97656.25 * 0.1 * dpi, false)
  //cam.moveToCenterPositionZoom(Coord(CoordinateSystemIdentifiers.EPSG4326() , 11.39, 47.26, 0.0), 97656.25 * 0.009 * dpi, false)
  

  val layer = Tiled2dMapVectorLayerInterface.createFromStyleJson(
          "name",
          //"https://static-prod.ubmeteo.io/v1/map/styles/light-globe/style.json",
          //"https://static-dev.ubmeteo.io/v1/map/styles/light-globe/style.json",
          //"https://static-dev.ubmeteo.io/v1/map/styles/colored-light-globe/style.json",
          //"https://demotiles.maplibre.org/style.json",
          //"http://localhost:8888/ctr-tma-style.json",
          //"https://demotiles.maplibre.org/styles/osm-bright-gl-style/style.json",
          //"http://localhost:8888/flesk.json",
          "http://localhost:8888/style.json",
          arrayListOf(DataLoader()),
          FontLoader(dpi)
  )

  val layeri = layer.asLayerInterface()
  //layeri.setErrorManager(MyErrorManager())

  // Add layer async and enforce running the corresponding task.
  mapInterface.resume()
  mapInterface.addLayer(layeri)
  while(mapInterface.getScheduler().runGraphicsTasks()) {}
  layeri.enableAnimations(false)
  cam.freeze(true);

  println(cam.getVisibleRect())
  //mapInterface.drawReadyFrame(cam.getVisibleRect(), 5f, MyMapReadyCallbackInterface())
  while(true) {
    while(mapInterface.getScheduler().runGraphicsTasks()) {}
    var readyState = layeri.isReadyToRenderOffscreen()
    if (readyState != LayerReadyState.NOT_READY) {
      break;
    }
    Thread.yield()
  }
  // Text icon fade in workaround:
  // - getVpMatrix must be called before first update.
  // - need two calls to update before first actual draw.
  //    -> first update runs symbol collision detection
  //    -> snd update to show non-colliding symbols.
  //    (and another one that is implicit in drawFrame?)
  cam.asCameraInterface().getVpMatrix()
  layeri.update();
  layeri.update();


  mapInterface.drawFrame()
  ImageIO.write(ctx.getImage(), "png", File("frame.png"))
  println("done")

  // Ok tile rendering
  val bbox = cam.getVisibleRect() // XXX, just for testing, needs to be system input

  val tileSize = 256;
  ctx.makeCurrent(tileSize, tileSize)
  mapInterface.setViewportSize(Vec2I(tileSize, tileSize))


  cam.freeze(false);
  println(bbox)
  for (z in 0..12) {
    assert(zooms[z].zoomLevelIdentifier == z)
    val (cols, rows) = tileRanges(bbox, zooms[z])

    println("range: ${z} ${cols} ${rows}")
    for (x in cols.first..cols.second) {
      for (y in rows.first..rows.second) {
        val tile = tileBBox(x, y, zooms[z])
        println("${x},${y}: ${tile}")


        cam.moveToBoundingBox(tile, 0.0f, false, zooms[z].zoom, zooms[z].zoom)
        awaitReady(mapInterface)
        mapInterface.drawFrame()
        ImageIO.write(ctx.getImage(), "png", File("tiles/tile_${z}_${x}_${y}.png"))
      }
    }
  }

  mapInterface.destroy()
}

fun tileRanges(bbox: RectCoord, m: Tiled2dMapZoomLevelInfo): Pair<Pair<Int, Int>, Pair<Int, Int>> 
{
  assert(bbox.topLeft.systemIdentifier == m.bounds.topLeft.systemIdentifier)
  val epsilon = 1e-6
  val tileSpan = m.tileWidthLayerSystemUnits; // == scaleDenominator * 0.00028 * 256
  
  val minCol = kotlin.math.floor((bbox.topLeft.x - m.bounds.topLeft.x) / tileSpan + epsilon).toInt()
  val maxCol = kotlin.math.floor((bbox.bottomRight.x - m.bounds.topLeft.x) / tileSpan - epsilon).toInt()
  val minRow = kotlin.math.floor((m.bounds.topLeft.y - bbox.topLeft.y) / tileSpan + epsilon).toInt()
  val maxRow = kotlin.math.floor((m.bounds.topLeft.y - bbox.bottomRight.y) / tileSpan - epsilon).toInt()
  return Pair(Pair(minCol, maxCol), Pair(minRow, maxRow))
}

fun tileBBox(col: Int, row: Int, m: Tiled2dMapZoomLevelInfo): RectCoord {
  val tileSpan = m.tileWidthLayerSystemUnits; // == scaleDenominator * 0.00028 * 256
  val leftX = col * tileSpan + m.bounds.topLeft.x;
  val rightX = (col+1) * tileSpan + m.bounds.topLeft.x;
  val upperY = m.bounds.topLeft.y - row * tileSpan;
  val lowerY = m.bounds.topLeft.y - (row+1) * tileSpan;
  return RectCoord(Coord(m.bounds.topLeft.systemIdentifier, leftX, upperY, 0.0),
                   Coord(m.bounds.topLeft.systemIdentifier, rightX, lowerY, 0.0))
}

fun awaitReady(map: MapInterface) {
  var layers = map.getLayers()
  while(true) {
    while(map.getScheduler().runGraphicsTasks()) {}
    var readyState = layers[0].isReadyToRenderOffscreen() // TODO for all
    if (readyState != LayerReadyState.NOT_READY) {
      break;
    }
    Thread.yield()
  }
}
