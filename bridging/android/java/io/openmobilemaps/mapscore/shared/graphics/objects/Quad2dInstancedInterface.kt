// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from graphicsobjects.djinni

package io.openmobilemaps.mapscore.shared.graphics.objects

import com.snapchat.djinni.NativeObjectManager
import java.util.concurrent.atomic.AtomicBoolean

abstract class Quad2dInstancedInterface {

    abstract fun setFrame(frame: io.openmobilemaps.mapscore.shared.graphics.common.Quad2dD, origin: io.openmobilemaps.mapscore.shared.graphics.common.Vec3D, is3d: Boolean)

    abstract fun setInstanceCount(count: Int)

    abstract fun setPositions(positions: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes)

    abstract fun setScales(scales: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes)

    abstract fun setRotations(rotations: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes)

    abstract fun setAlphas(values: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes)

    abstract fun setTextureCoordinates(textureCoordinates: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes)

    /**
     * 2 floats (x and y) for each instance
     * defines the offset applied to the projected position in viewspace coordinates
     */
    abstract fun setPositionOffset(offsets: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes)

    abstract fun loadTexture(context: io.openmobilemaps.mapscore.shared.graphics.RenderingContextInterface, textureHolder: TextureHolderInterface)

    abstract fun removeTexture()

    abstract fun asGraphicsObject(): GraphicsObjectInterface

    abstract fun asMaskingObject(): MaskingObjectInterface

    public class CppProxy : Quad2dInstancedInterface {
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

        override fun setFrame(frame: io.openmobilemaps.mapscore.shared.graphics.common.Quad2dD, origin: io.openmobilemaps.mapscore.shared.graphics.common.Vec3D, is3d: Boolean) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_setFrame(this.nativeRef, frame, origin, is3d)
        }
        private external fun native_setFrame(_nativeRef: Long, frame: io.openmobilemaps.mapscore.shared.graphics.common.Quad2dD, origin: io.openmobilemaps.mapscore.shared.graphics.common.Vec3D, is3d: Boolean)

        override fun setInstanceCount(count: Int) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_setInstanceCount(this.nativeRef, count)
        }
        private external fun native_setInstanceCount(_nativeRef: Long, count: Int)

        override fun setPositions(positions: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_setPositions(this.nativeRef, positions)
        }
        private external fun native_setPositions(_nativeRef: Long, positions: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes)

        override fun setScales(scales: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_setScales(this.nativeRef, scales)
        }
        private external fun native_setScales(_nativeRef: Long, scales: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes)

        override fun setRotations(rotations: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_setRotations(this.nativeRef, rotations)
        }
        private external fun native_setRotations(_nativeRef: Long, rotations: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes)

        override fun setAlphas(values: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_setAlphas(this.nativeRef, values)
        }
        private external fun native_setAlphas(_nativeRef: Long, values: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes)

        override fun setTextureCoordinates(textureCoordinates: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_setTextureCoordinates(this.nativeRef, textureCoordinates)
        }
        private external fun native_setTextureCoordinates(_nativeRef: Long, textureCoordinates: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes)

        override fun setPositionOffset(offsets: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_setPositionOffset(this.nativeRef, offsets)
        }
        private external fun native_setPositionOffset(_nativeRef: Long, offsets: io.openmobilemaps.mapscore.shared.graphics.common.SharedBytes)

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

        override fun asMaskingObject(): MaskingObjectInterface {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_asMaskingObject(this.nativeRef)
        }
        private external fun native_asMaskingObject(_nativeRef: Long): MaskingObjectInterface
    }
}
