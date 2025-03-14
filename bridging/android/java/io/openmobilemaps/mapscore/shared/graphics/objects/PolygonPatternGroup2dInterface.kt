// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from graphicsobjects.djinni

package io.openmobilemaps.mapscore.shared.graphics.objects

import com.snapchat.djinni.NativeObjectManager
import java.util.concurrent.atomic.AtomicBoolean

abstract class PolygonPatternGroup2dInterface {

    abstract fun setVertices(vertices: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes, indices: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes, origin: io.openmobilemaps.mapscore.shared.graphics.common.Vec3D)

    abstract fun setOpacities(values: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes)

    /** 4 floats (x, y, width and height) in uv space and 2 uint_16 values for the pixel with and height for each instanced */
    abstract fun setTextureCoordinates(values: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes)

    abstract fun setScalingFactor(factor: Float)

    abstract fun setScalingFactors(factor: io.openmobilemaps.mapscore.shared.graphics.common.Vec2F)

    abstract fun loadTexture(context: io.openmobilemaps.mapscore.shared.graphics.RenderingContextInterface, textureHolder: TextureHolderInterface)

    abstract fun removeTexture()

    abstract fun asGraphicsObject(): GraphicsObjectInterface

    public class CppProxy : PolygonPatternGroup2dInterface {
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

        override fun setVertices(vertices: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes, indices: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes, origin: io.openmobilemaps.mapscore.shared.graphics.common.Vec3D) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_setVertices(this.nativeRef, vertices, indices, origin)
        }
        private external fun native_setVertices(_nativeRef: Long, vertices: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes, indices: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes, origin: io.openmobilemaps.mapscore.shared.graphics.common.Vec3D)

        override fun setOpacities(values: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_setOpacities(this.nativeRef, values)
        }
        private external fun native_setOpacities(_nativeRef: Long, values: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes)

        override fun setTextureCoordinates(values: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_setTextureCoordinates(this.nativeRef, values)
        }
        private external fun native_setTextureCoordinates(_nativeRef: Long, values: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes)

        override fun setScalingFactor(factor: Float) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_setScalingFactor(this.nativeRef, factor)
        }
        private external fun native_setScalingFactor(_nativeRef: Long, factor: Float)

        override fun setScalingFactors(factor: io.openmobilemaps.mapscore.shared.graphics.common.Vec2F) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_setScalingFactors(this.nativeRef, factor)
        }
        private external fun native_setScalingFactors(_nativeRef: Long, factor: io.openmobilemaps.mapscore.shared.graphics.common.Vec2F)

        override fun loadTexture(context: io.openmobilemaps.mapscore.shared.graphics.RenderingContextInterface, textureHolder: TextureHolderInterface) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_loadTexture(this.nativeRef, context, textureHolder)
        }
        private external fun native_loadTexture(_nativeRef: Long, context: io.openmobilemaps.mapscore.shared.graphics.RenderingContextInterface, textureHolder: TextureHolderInterface)

        override fun removeTexture() {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_removeTexture(this.nativeRef)
        }
        private external fun native_removeTexture(_nativeRef: Long)

        override fun asGraphicsObject(): GraphicsObjectInterface {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_asGraphicsObject(this.nativeRef)
        }
        private external fun native_asGraphicsObject(_nativeRef: Long): GraphicsObjectInterface
    }
}
