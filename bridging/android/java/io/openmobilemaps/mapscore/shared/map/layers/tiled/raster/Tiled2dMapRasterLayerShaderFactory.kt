// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from tiled_raster_layer.djinni

package io.openmobilemaps.mapscore.shared.map.layers.tiled.raster

import java.util.concurrent.atomic.AtomicBoolean

abstract class Tiled2dMapRasterLayerShaderFactory {

    abstract fun combineShader(): io.openmobilemaps.mapscore.shared.graphics.shader.AlphaShaderInterface

    abstract fun finalShader(): io.openmobilemaps.mapscore.shared.graphics.shader.InterpolationShaderInterface

    private class CppProxy : Tiled2dMapRasterLayerShaderFactory {
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

        override fun combineShader(): io.openmobilemaps.mapscore.shared.graphics.shader.AlphaShaderInterface {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_combineShader(this.nativeRef)
        }
        private external fun native_combineShader(_nativeRef: Long): io.openmobilemaps.mapscore.shared.graphics.shader.AlphaShaderInterface

        override fun finalShader(): io.openmobilemaps.mapscore.shared.graphics.shader.InterpolationShaderInterface {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_finalShader(this.nativeRef)
        }
        private external fun native_finalShader(_nativeRef: Long): io.openmobilemaps.mapscore.shared.graphics.shader.InterpolationShaderInterface
    }
}