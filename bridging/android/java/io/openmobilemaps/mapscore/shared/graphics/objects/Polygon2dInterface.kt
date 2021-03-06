// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from graphicsobjects.djinni

package io.openmobilemaps.mapscore.shared.graphics.objects

import java.util.concurrent.atomic.AtomicBoolean

abstract class Polygon2dInterface {

    abstract fun setPolygonPositions(positions: ArrayList<io.openmobilemaps.mapscore.shared.graphics.common.Vec2D>, holes: ArrayList<ArrayList<io.openmobilemaps.mapscore.shared.graphics.common.Vec2D>>, isConvex: Boolean)

    abstract fun asGraphicsObject(): GraphicsObjectInterface

    private class CppProxy : Polygon2dInterface {
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

        override fun setPolygonPositions(positions: ArrayList<io.openmobilemaps.mapscore.shared.graphics.common.Vec2D>, holes: ArrayList<ArrayList<io.openmobilemaps.mapscore.shared.graphics.common.Vec2D>>, isConvex: Boolean) {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            native_setPolygonPositions(this.nativeRef, positions, holes, isConvex)
        }
        private external fun native_setPolygonPositions(_nativeRef: Long, positions: ArrayList<io.openmobilemaps.mapscore.shared.graphics.common.Vec2D>, holes: ArrayList<ArrayList<io.openmobilemaps.mapscore.shared.graphics.common.Vec2D>>, isConvex: Boolean)

        override fun asGraphicsObject(): GraphicsObjectInterface {
            assert(!this.destroyed.get()) { error("trying to use a destroyed object") }
            return native_asGraphicsObject(this.nativeRef)
        }
        private external fun native_asGraphicsObject(_nativeRef: Long): GraphicsObjectInterface
    }
}
