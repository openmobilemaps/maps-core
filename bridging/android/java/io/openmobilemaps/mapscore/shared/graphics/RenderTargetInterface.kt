// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from core.djinni

package io.openmobilemaps.mapscore.shared.graphics

import com.snapchat.djinni.NativeObjectManager
import java.util.concurrent.atomic.AtomicBoolean

abstract class RenderTargetInterface {

    abstract fun asGlRenderTargetInterface(): OpenGlRenderTargetInterface?

    public class CppProxy : RenderTargetInterface {
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

        override fun asGlRenderTargetInterface(): OpenGlRenderTargetInterface? {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_asGlRenderTargetInterface(this.nativeRef)
        }
        private external fun native_asGlRenderTargetInterface(_nativeRef: Long): OpenGlRenderTargetInterface?
    }
}
