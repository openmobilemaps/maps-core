// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from shader.djinni

package io.openmobilemaps.mapscore.shared.graphics.shader

import com.snapchat.djinni.NativeObjectManager
import java.util.concurrent.atomic.AtomicBoolean

abstract class SphereProjectionShaderInterface {

    abstract fun updateAlpha(value: Float)

    abstract fun asShaderProgramInterface(): ShaderProgramInterface

    private class CppProxy : SphereProjectionShaderInterface {
        private val nativeRef: Long
        private val destroyed: AtomicBoolean = AtomicBoolean(false)

        private constructor(nativeRef: Long) {
            if (nativeRef == 0L) error("nativeRef is zero")
            this.nativeRef = nativeRef
            NativeObjectManager.register(this, nativeRef)
        }

        external fun nativeDestroy(nativeRef: Long)

        override fun updateAlpha(value: Float) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_updateAlpha(this.nativeRef, value)
        }
        private external fun native_updateAlpha(_nativeRef: Long, value: Float)

        override fun asShaderProgramInterface(): ShaderProgramInterface {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_asShaderProgramInterface(this.nativeRef)
        }
        private external fun native_asShaderProgramInterface(_nativeRef: Long): ShaderProgramInterface
    }
}