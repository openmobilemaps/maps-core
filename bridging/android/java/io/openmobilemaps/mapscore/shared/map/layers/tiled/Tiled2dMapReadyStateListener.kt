// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from tiled_layer.djinni

package io.openmobilemaps.mapscore.shared.map.layers.tiled

import com.snapchat.djinni.NativeObjectManager
import java.util.concurrent.atomic.AtomicBoolean

abstract class Tiled2dMapReadyStateListener {

    abstract fun stateUpdate(state: io.openmobilemaps.mapscore.shared.map.LayerReadyState)

    public class CppProxy : Tiled2dMapReadyStateListener {
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

        override fun stateUpdate(state: io.openmobilemaps.mapscore.shared.map.LayerReadyState) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_stateUpdate(this.nativeRef, state)
        }
        private external fun native_stateUpdate(_nativeRef: Long, state: io.openmobilemaps.mapscore.shared.map.LayerReadyState)
    }
}
