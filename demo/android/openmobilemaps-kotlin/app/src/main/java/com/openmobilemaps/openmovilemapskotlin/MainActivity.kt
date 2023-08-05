package com.openmobilemaps.openmovilemapskotlin

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import io.openmobilemaps.mapscore.MapsCore
import io.openmobilemaps.mapscore.map.loader.DataLoader
import io.openmobilemaps.mapscore.map.view.MapView
import io.openmobilemaps.mapscore.shared.map.MapConfig
import io.openmobilemaps.mapscore.shared.map.coordinates.Coord
import io.openmobilemaps.mapscore.shared.map.coordinates.CoordinateSystemFactory
import io.openmobilemaps.mapscore.shared.map.coordinates.CoordinateSystemIdentifiers
import io.openmobilemaps.mapscore.shared.map.coordinates.RectCoord
import io.openmobilemaps.mapscore.shared.map.layers.tiled.Tiled2dMapLayerConfig
import io.openmobilemaps.mapscore.shared.map.layers.tiled.Tiled2dMapVectorSettings
import io.openmobilemaps.mapscore.shared.map.layers.tiled.Tiled2dMapZoomInfo
import io.openmobilemaps.mapscore.shared.map.layers.tiled.Tiled2dMapZoomLevelInfo
import io.openmobilemaps.mapscore.shared.map.layers.tiled.raster.Tiled2dMapRasterLayerInterface
import io.openmobilemaps.mapscore.shared.map.loader.LoaderInterface

class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        MapsCore.initialize()

        val mapView = findViewById<MapView>(R.id.MapView)

        mapView.setupMap(MapConfig(CoordinateSystemFactory.getEpsg3857System()))

        mapView.registerLifecycle(lifecycle)

        val layerConfig = object : Tiled2dMapLayerConfig() {
            // Defines the bounds of the layer and implicitly the coordinate system used by the layer as well
            val epsg3857Bounds: RectCoord = RectCoord(
                Coord(CoordinateSystemIdentifiers.EPSG3857(), -20037508.34, 20037508.34, 0.0),
                Coord(CoordinateSystemIdentifiers.EPSG3857(), 20037508.34, -20037508.34, 0.0)
            )

            // Defines to map coordinate system of the layer
            override fun getCoordinateSystemIdentifier() : String = CoordinateSystemIdentifiers.EPSG3857()

            // Name of the layer
            override fun getLayerName(): String = "OSMLayer"

            override fun getVectorSettings(): Tiled2dMapVectorSettings? {
                TODO("Not yet implemented")
            }

            override fun getZoomInfo(): Tiled2dMapZoomInfo = Tiled2dMapZoomInfo(
                zoomLevelScaleFactor = 0.6f,
                numDrawPreviousLayers = 2,
                adaptScaleToScreen = true,
                maskTile = true,
                underzoom = true,
                overzoom = true
            )

            override fun getTileUrl(x: Int, y: Int, t: Int, zoom: Int): String {
                var url = "https://tile.openstreetmap.org/$zoom/$x/$y.png"
//                url = "https://webrd01.is.autonavi.com/appmaptile?lang=zh_cn&size=1&scale=1&style=8&x=$x&y=$y&z=$zoom"
//                Log.d("getTileUrl", url)
                return url
            }

            override fun getZoomLevelInfos(): ArrayList<Tiled2dMapZoomLevelInfo> = ArrayList(
                listOf(
                    Tiled2dMapZoomLevelInfo(559082264.029, 40075016f, 1, 1, 1, 0, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(279541132.015, 20037508f, 2, 2, 1, 1, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(139770566.007, 10018754f, 4, 4, 1, 2, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(69885283.0036, 5009377.1f, 8, 8, 1, 3, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(34942641.5018, 2504688.5f, 16, 16, 1, 4, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(17471320.7509, 1252344.3f, 32, 32, 1, 5, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(8735660.37545, 626172.1f, 64, 64, 1, 6, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(4367830.18773, 313086.1f, 128, 128, 1, 7, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(2183915.09386, 156543f, 256, 256, 1, 8, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(1091957.54693, 78271.5f, 512, 512, 1, 9, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(545978.773466, 39135.8f, 1024, 1024, 1, 10, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(272989.386733, 19567.9f, 2048, 2048, 1, 11, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(136494.693366, 9783.94f, 4096, 4096, 1, 12, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(68247.3466832, 4891.97f, 8192, 8192, 1, 13, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(34123.6733416, 2445.98f, 16384, 16384, 1, 14, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(17061.8366708, 1222.99f, 32768, 32768, 1, 15, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(8530.91833540, 611.496f, 65536, 65536, 1, 16, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(4265.45916770, 305.748f, 131072, 131072, 1, 17, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(2132.72958385, 152.874f, 262144, 262144, 1, 18, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(1066.36479193, 76.437f, 524288, 524288, 1, 19, epsg3857Bounds),
                    Tiled2dMapZoomLevelInfo(533.18239597, 38.2185f, 1_048_576, 1_048_576, 1, 19, epsg3857Bounds)
                )
            )
        }

        val userAgent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0.0.0 Safari/537.36"
        var referrer = "null"
        val textureLoader = DataLoader(this, cacheDir, 50L * 1024L * 1024L, referrer, userAgent)
        val texture = ArrayList<LoaderInterface>()
        texture.add(textureLoader)
        val tiledLayer = Tiled2dMapRasterLayerInterface.create(layerConfig, texture)

        mapView.addLayer(tiledLayer.asLayerInterface())
    }
}