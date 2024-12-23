// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from wmts_capabilities.djinni

package io.openmobilemaps.mapscore.shared.map.layers.tiled.raster.wmts

import com.snapchat.djinni.NativeObjectManager
import java.util.concurrent.atomic.AtomicBoolean

abstract class WmtsTiled2dMapLayerConfigFactory {

    companion object {
        @JvmStatic
        external fun create(wmtsLayerConfiguration: WmtsLayerDescription, zoomLevelInfo: ArrayList<io.openmobilemaps.mapscore.shared.map.layers.tiled.Tiled2dMapZoomLevelInfo>, zoomInfo: io.openmobilemaps.mapscore.shared.map.layers.tiled.Tiled2dMapZoomInfo, coordinateSystemIdentifier: Int, matrixSetIdentifier: String): io.openmobilemaps.mapscore.shared.map.layers.tiled.Tiled2dMapLayerConfig
    }

    public class CppProxy : WmtsTiled2dMapLayerConfigFactory {
        private val nativeRef: Long
        private val destroyed: AtomicBoolean = AtomicBoolean(false)

        private constructor(nativeRef: Long) {
            if (nativeRef == 0L) error("nativeRef is zero")
            this.nativeRef = nativeRef
            NativeObjectManager.register(this, nativeRef)
        }

        companion object {
            @JvmStatic
            external fun nativeDestroy(nativeRef: Long)
        }
    }
}
