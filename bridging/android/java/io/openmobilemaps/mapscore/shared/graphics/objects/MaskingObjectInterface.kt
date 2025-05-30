// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from graphicsobjects.djinni

package io.openmobilemaps.mapscore.shared.graphics.objects

import com.snapchat.djinni.NativeObjectManager
import java.util.concurrent.atomic.AtomicBoolean

abstract class MaskingObjectInterface {

    abstract fun asGraphicsObject(): GraphicsObjectInterface

    abstract fun renderAsMask(context: io.openmobilemaps.mapscore.shared.graphics.RenderingContextInterface, renderPass: io.openmobilemaps.mapscore.shared.graphics.RenderPassConfig, vpMatrix: Long, mMatrix: Long, origin: io.openmobilemaps.mapscore.shared.graphics.common.Vec3D, screenPixelAsRealMeterFactor: Double)

    public class CppProxy : MaskingObjectInterface {
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

        override fun asGraphicsObject(): GraphicsObjectInterface {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_asGraphicsObject(this.nativeRef)
        }
        private external fun native_asGraphicsObject(_nativeRef: Long): GraphicsObjectInterface

        override fun renderAsMask(context: io.openmobilemaps.mapscore.shared.graphics.RenderingContextInterface, renderPass: io.openmobilemaps.mapscore.shared.graphics.RenderPassConfig, vpMatrix: Long, mMatrix: Long, origin: io.openmobilemaps.mapscore.shared.graphics.common.Vec3D, screenPixelAsRealMeterFactor: Double) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_renderAsMask(this.nativeRef, context, renderPass, vpMatrix, mMatrix, origin, screenPixelAsRealMeterFactor)
        }
        private external fun native_renderAsMask(_nativeRef: Long, context: io.openmobilemaps.mapscore.shared.graphics.RenderingContextInterface, renderPass: io.openmobilemaps.mapscore.shared.graphics.RenderPassConfig, vpMatrix: Long, mMatrix: Long, origin: io.openmobilemaps.mapscore.shared.graphics.common.Vec3D, screenPixelAsRealMeterFactor: Double)
    }
}
