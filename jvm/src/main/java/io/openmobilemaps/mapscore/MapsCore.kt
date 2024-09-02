package io.openmobilemaps.mapscore

import io.openmobilemaps.mapscore.graphics.util.OSMesa
import io.openmobilemaps.mapscore.shared.graphics.common.Color
import io.openmobilemaps.mapscore.shared.graphics.common.Vec2I
import io.openmobilemaps.mapscore.shared.map.MapCallbackInterface
import io.openmobilemaps.mapscore.shared.map.MapCamera2dInterface
import io.openmobilemaps.mapscore.shared.map.MapConfig
import io.openmobilemaps.mapscore.shared.map.MapInterface
import io.openmobilemaps.mapscore.shared.map.coordinates.CoordinateSystemFactory
import java.io.File
import javax.imageio.ImageIO

object MapsCore {
  fun initialize() {
    // TODO: use Native blabla loader from Djinni Java support library?
    with(createTempFile(prefix = "libmapscore_jni_", suffix = ".so")) {
      deleteOnExit()
      val bytes = MapsCore::class.java.getResource("/native/libmapscore_jni.so").readBytes()
      outputStream().write(bytes)
      System.load(path)
    }
  }
}

fun main() {
  System.out.println("hi")
  MapsCore.initialize()
  var ctx = OSMesa(500, 500)

  val config = MapConfig(CoordinateSystemFactory.getEpsg2056System())
  System.out.println(config)

  val mapInterface = MapInterface.createWithOpenGl(config, 1.0f)
  System.out.println("map: ${mapInterface}")
  mapInterface.setCallbackHandler(
          object : MapCallbackInterface() {
            override fun invalidate() {}
            override fun onMapResumed() {}
          }
  )

  mapInterface.getRenderingContext().onSurfaceCreated()

  mapInterface.setViewportSize(Vec2I(500, 500))
  mapInterface.setBackgroundColor(Color(0.4f, 0.4f, 1f, 1f))
  mapInterface.setCamera(MapCamera2dInterface.create(mapInterface, 1.0f))
  mapInterface.resume()
  mapInterface.drawFrame()

  var img = ctx.getImage()
  ImageIO.write(img, "png", File("frame.png"))

  try {} finally {
    mapInterface.destroy()
  }
}
