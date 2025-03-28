// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from icon.djinni

package io.openmobilemaps.mapscore.shared.map.layers.icon

import com.snapchat.djinni.NativeObjectManager
import java.util.concurrent.atomic.AtomicBoolean

abstract class IconLayerCallbackInterface {

    abstract fun onClickConfirmed(icons: ArrayList<IconInfoInterface>): Boolean

    abstract fun onLongPress(icons: ArrayList<IconInfoInterface>): Boolean

    public class CppProxy : IconLayerCallbackInterface {
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

        override fun onClickConfirmed(icons: ArrayList<IconInfoInterface>): Boolean {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_onClickConfirmed(this.nativeRef, icons)
        }
        private external fun native_onClickConfirmed(_nativeRef: Long, icons: ArrayList<IconInfoInterface>): Boolean

        override fun onLongPress(icons: ArrayList<IconInfoInterface>): Boolean {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_onLongPress(this.nativeRef, icons)
        }
        private external fun native_onLongPress(_nativeRef: Long, icons: ArrayList<IconInfoInterface>): Boolean
    }
}
