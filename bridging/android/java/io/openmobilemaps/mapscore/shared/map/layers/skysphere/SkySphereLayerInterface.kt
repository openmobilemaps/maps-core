// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from sky_sphere.djinni

package io.openmobilemaps.mapscore.shared.map.layers.skysphere

import com.snapchat.djinni.NativeObjectManager
import java.util.concurrent.atomic.AtomicBoolean

abstract class SkySphereLayerInterface {

    companion object {
        @JvmStatic
        external fun create(): SkySphereLayerInterface
    }

    abstract fun asLayerInterface(): io.openmobilemaps.mapscore.shared.map.LayerInterface

    /** Expects a texture with x: right ascension (longitude), y: declination (latitude) */
    abstract fun setTexture(texture: io.openmobilemaps.mapscore.shared.graphics.objects.TextureHolderInterface)

    public class CppProxy : SkySphereLayerInterface {
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

        override fun asLayerInterface(): io.openmobilemaps.mapscore.shared.map.LayerInterface {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_asLayerInterface(this.nativeRef)
        }
        private external fun native_asLayerInterface(_nativeRef: Long): io.openmobilemaps.mapscore.shared.map.LayerInterface

        override fun setTexture(texture: io.openmobilemaps.mapscore.shared.graphics.objects.TextureHolderInterface) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_setTexture(this.nativeRef, texture)
        }
        private external fun native_setTexture(_nativeRef: Long, texture: io.openmobilemaps.mapscore.shared.graphics.objects.TextureHolderInterface)
    }
}
