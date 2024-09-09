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
  var dpi = 2*96.0;
  var size = 2000;
  var ctx = OSMesa(size, size)

  val config = MapConfig(CoordinateSystemFactory.getEpsg3857System())

  val mapInterface = MapInterface.createWithOpenGl(config, dpi.toFloat())
  mapInterface.setCallbackHandler(
          object : MapCallbackInterface() {
            override fun invalidate() {}
            override fun onMapResumed() {}
          }
  )

  mapInterface.getRenderingContext().onSurfaceCreated()

  mapInterface.setViewportSize(Vec2I(size, size))
  mapInterface.setBackgroundColor(Color(0.2f, 0.1f, 0.1f, 0.1f))
  var cam = mapInterface.getCamera()

  // var bbox = RectCoord(
  //   topLeft=Coord(CoordinateSystemIdentifiers.EPSG4326(), 32.48107654411925, 44.38083293528811, 0.0),
  //   bottomRight=Coord(CoordinateSystemIdentifiers.EPSG4326(), 36.637536777859964, 46.55925987559425, 0.0),
  // )
  // cam.moveToBoundingBox(bbox, 0.1f, false, null, null);
  //cam.moveToCenterPositionZoom(Coord(CoordinateSystemIdentifiers.EPSG4326() , 34.00905273547181, 46.55925987559425, 0.0), 200.0, false)
  cam.moveToCenterPositionZoom(Coord(CoordinateSystemIdentifiers.EPSG4326() , 8.2, 47.0, 0.0), 97656.25 * 0.1 * dpi, false)
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
  while(mapInterface.getLayers().isEmpty()) {
     mapInterface.getScheduler().runGraphicsTasks()
  }
  layeri.enableAnimations(false)
  cam.freeze(true);

  println(cam.getVisibleRect())
  //mapInterface.drawReadyFrame(cam.getVisibleRect(), 5f, MyMapReadyCallbackInterface())
  for(i in 0..500) {
    mapInterface.getScheduler().runGraphicsTasks()
    var readyState = layeri.isReadyToRenderOffscreen()
    println("readyState: ${readyState}")
    if (readyState != LayerReadyState.NOT_READY) {
      break;
    }
    java.util.concurrent.TimeUnit.MILLISECONDS.sleep(10);
  }
  //XXX: workaround for fade in of text labels. :*(
  //  Set transition.duration = 0 in style.json, otherwise this needs to be even longer.
  for(i in 0..1) {
    mapInterface.drawFrame()
    java.util.concurrent.TimeUnit.MILLISECONDS.sleep(10);
  }
  mapInterface.drawFrame()



  var img = ctx.getImage()
  ImageIO.write(img, "png", File("frame.png"))
  println("done")


  try {} finally {
    mapInterface.destroy()
  }
}
