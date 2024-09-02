package io.openmobilemaps.mapscore

import io.openmobilemaps.mapscore.shared.graphics.common.Color
import io.openmobilemaps.mapscore.shared.graphics.common.Vec2I
import io.openmobilemaps.mapscore.shared.map.MapCallbackInterface
import io.openmobilemaps.mapscore.shared.map.MapConfig
import io.openmobilemaps.mapscore.shared.map.MapInterface
import io.openmobilemaps.mapscore.shared.map.MapCamera2dInterface
import io.openmobilemaps.mapscore.shared.map.coordinates.CoordinateSystemFactory

object MapsCore {
  fun initialize() {
    // TODO: use Native blabla loader from Djinni Java support library?
    with(createTempFile(prefix = "libmapscore_", suffix = ".so")) {
      deleteOnExit()
      val bytes = MapsCore::class.java.getResource("/native/libmapscore.so").readBytes()
      outputStream().write(bytes)
      System.load(path)
    }
  }
}

fun main() {
  System.out.println("hi")
  MapsCore.initialize()

  val config = MapConfig(CoordinateSystemFactory.getEpsg2056System())
  System.out.println(config)

  val mapInterface = MapInterface.createWithOpenGl(config, 1.0f)
  System.out.println("map: ${mapInterface}")
  mapInterface.setCallbackHandler(
          object : MapCallbackInterface() {
            override fun invalidate() {
              // TODO
              // glThread.requestRender()
            }
            override fun onMapResumed() {}
          }
  )
  GLThread.initFNORD()

  mapInterface.getRenderingContext().onSurfaceCreated()

  mapInterface.setViewportSize(Vec2I(300, 300))
  mapInterface.setBackgroundColor(Color(0f, 1f, 1f, 1f))
  mapInterface.setCamera(MapCamera2dInterface.create(mapInterface, 1.0f));

  mapInterface.resume()
  mapInterface.drawFrame()

  try {
    GLThread.doneFNORD()
  } finally {
    mapInterface.destroy()
  }
}
