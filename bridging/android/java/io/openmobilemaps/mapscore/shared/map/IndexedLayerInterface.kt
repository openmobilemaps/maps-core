// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from core.djinni

package io.openmobilemaps.mapscore.shared.map

import com.snapchat.djinni.NativeObjectManager
import java.util.concurrent.atomic.AtomicBoolean

abstract class IndexedLayerInterface {

    abstract fun getLayerInterface(): LayerInterface

    abstract fun getIndex(): Int

    public class CppProxy : IndexedLayerInterface {
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

        override fun getLayerInterface(): LayerInterface {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_getLayerInterface(this.nativeRef)
        }
        private external fun native_getLayerInterface(_nativeRef: Long): LayerInterface

        override fun getIndex(): Int {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_getIndex(this.nativeRef)
        }
        private external fun native_getIndex(_nativeRef: Long): Int
    }
}
