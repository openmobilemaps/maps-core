// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from polygon.djinni

package io.openmobilemaps.mapscore.shared.map.layers.polygon

import java.util.concurrent.atomic.AtomicBoolean

abstract class PolygonLayerCallbackInterface {

    abstract fun onClickConfirmed(polygon: PolygonInfo)

    private class CppProxy : PolygonLayerCallbackInterface {
        private val nativeRef: Long
        private val destroyed: AtomicBoolean = AtomicBoolean(false)

        private constructor(nativeRef: Long) {
            if (nativeRef == 0L) error("nativeRef is zero")
            this.nativeRef = nativeRef
        }

        private external fun nativeDestroy(nativeRef: Long)
        fun _djinni_private_destroy() {
            val destroyed = this.destroyed.getAndSet(true)
            if (!destroyed) nativeDestroy(this.nativeRef)
        }
        protected fun finalize() {
            _djinni_private_destroy()
        }

        override fun onClickConfirmed(polygon: PolygonInfo) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_onClickConfirmed(this.nativeRef, polygon)
        }
        private external fun native_onClickConfirmed(_nativeRef: Long, polygon: PolygonInfo)
    }
}
