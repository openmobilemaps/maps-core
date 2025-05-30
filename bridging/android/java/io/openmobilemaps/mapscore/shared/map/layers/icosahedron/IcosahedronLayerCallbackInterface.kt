// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from icosahedron.djinni

package io.openmobilemaps.mapscore.shared.map.layers.icosahedron

import com.snapchat.djinni.NativeObjectManager
import java.util.concurrent.atomic.AtomicBoolean

abstract class IcosahedronLayerCallbackInterface {

    abstract fun getData(): com.snapchat.djinni.Future<java.nio.ByteBuffer>

    public class CppProxy : IcosahedronLayerCallbackInterface {
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

        override fun getData(): com.snapchat.djinni.Future<java.nio.ByteBuffer> {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_getData(this.nativeRef)
        }
        private external fun native_getData(_nativeRef: Long): com.snapchat.djinni.Future<java.nio.ByteBuffer>
    }
}
