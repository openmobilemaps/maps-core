// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from reverse_geocoder.djinni

package io.openmobilemaps.mapscore.shared.utils

import com.snapchat.djinni.NativeObjectManager
import java.util.concurrent.atomic.AtomicBoolean

abstract class ReverseGeocoderInterface {

    companion object {
        @JvmStatic
        external fun create(loader: io.openmobilemaps.mapscore.shared.map.loader.LoaderInterface, tileUrlTemplate: String, zoomLevel: Int): ReverseGeocoderInterface
    }

    abstract fun reverseGeocode(coord: io.openmobilemaps.mapscore.shared.map.coordinates.Coord, thresholdMeters: Long): ArrayList<io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.VectorLayerFeatureCoordInfo>

    abstract fun reverseGeocodeClosest(coord: io.openmobilemaps.mapscore.shared.map.coordinates.Coord, thresholdMeters: Long): io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.VectorLayerFeatureCoordInfo?

    public class CppProxy : ReverseGeocoderInterface {
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

        override fun reverseGeocode(coord: io.openmobilemaps.mapscore.shared.map.coordinates.Coord, thresholdMeters: Long): ArrayList<io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.VectorLayerFeatureCoordInfo> {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_reverseGeocode(this.nativeRef, coord, thresholdMeters)
        }
        private external fun native_reverseGeocode(_nativeRef: Long, coord: io.openmobilemaps.mapscore.shared.map.coordinates.Coord, thresholdMeters: Long): ArrayList<io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.VectorLayerFeatureCoordInfo>

        override fun reverseGeocodeClosest(coord: io.openmobilemaps.mapscore.shared.map.coordinates.Coord, thresholdMeters: Long): io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.VectorLayerFeatureCoordInfo? {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_reverseGeocodeClosest(this.nativeRef, coord, thresholdMeters)
        }
        private external fun native_reverseGeocodeClosest(_nativeRef: Long, coord: io.openmobilemaps.mapscore.shared.map.coordinates.Coord, thresholdMeters: Long): io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.VectorLayerFeatureCoordInfo?
    }
}
