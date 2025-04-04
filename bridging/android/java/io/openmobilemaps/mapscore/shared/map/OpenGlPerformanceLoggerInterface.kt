// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from map_helpers.djinni

package io.openmobilemaps.mapscore.shared.map

import com.snapchat.djinni.NativeObjectManager
import java.util.concurrent.atomic.AtomicBoolean

abstract class OpenGlPerformanceLoggerInterface {

    companion object {
        @JvmStatic
        external fun create(): OpenGlPerformanceLoggerInterface

        @JvmStatic
        external fun createSpecifically(numBuckets: Int, bucketSizeMs: Long): OpenGlPerformanceLoggerInterface
    }

    abstract fun asPerformanceLoggerInterface(): PerformanceLoggerInterface

    public class CppProxy : OpenGlPerformanceLoggerInterface {
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

        override fun asPerformanceLoggerInterface(): PerformanceLoggerInterface {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_asPerformanceLoggerInterface(this.nativeRef)
        }
        private external fun native_asPerformanceLoggerInterface(_nativeRef: Long): PerformanceLoggerInterface
    }
}
