// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from tiled_vector_layer.djinni

package io.openmobilemaps.mapscore.shared.map.layers.tiled.vector

import com.snapchat.djinni.NativeObjectManager
import java.util.concurrent.atomic.AtomicBoolean

abstract class Tiled2dMapVectorLayerSelectionCallbackInterface {

    abstract fun didSelectFeature(featureInfo: VectorLayerFeatureInfo, layerIdentifier: String, coord: io.openmobilemaps.mapscore.shared.map.coordinates.Coord): Boolean

    abstract fun didMultiSelectLayerFeatures(featureInfos: ArrayList<VectorLayerFeatureInfo>, layerIdentifier: String, coord: io.openmobilemaps.mapscore.shared.map.coordinates.Coord): Boolean

    abstract fun didClickBackgroundConfirmed(coord: io.openmobilemaps.mapscore.shared.map.coordinates.Coord): Boolean

    public class CppProxy : Tiled2dMapVectorLayerSelectionCallbackInterface {
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

        override fun didSelectFeature(featureInfo: VectorLayerFeatureInfo, layerIdentifier: String, coord: io.openmobilemaps.mapscore.shared.map.coordinates.Coord): Boolean {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_didSelectFeature(this.nativeRef, featureInfo, layerIdentifier, coord)
        }
        private external fun native_didSelectFeature(_nativeRef: Long, featureInfo: VectorLayerFeatureInfo, layerIdentifier: String, coord: io.openmobilemaps.mapscore.shared.map.coordinates.Coord): Boolean

        override fun didMultiSelectLayerFeatures(featureInfos: ArrayList<VectorLayerFeatureInfo>, layerIdentifier: String, coord: io.openmobilemaps.mapscore.shared.map.coordinates.Coord): Boolean {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_didMultiSelectLayerFeatures(this.nativeRef, featureInfos, layerIdentifier, coord)
        }
        private external fun native_didMultiSelectLayerFeatures(_nativeRef: Long, featureInfos: ArrayList<VectorLayerFeatureInfo>, layerIdentifier: String, coord: io.openmobilemaps.mapscore.shared.map.coordinates.Coord): Boolean

        override fun didClickBackgroundConfirmed(coord: io.openmobilemaps.mapscore.shared.map.coordinates.Coord): Boolean {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_didClickBackgroundConfirmed(this.nativeRef, coord)
        }
        private external fun native_didClickBackgroundConfirmed(_nativeRef: Long, coord: io.openmobilemaps.mapscore.shared.map.coordinates.Coord): Boolean
    }
}
