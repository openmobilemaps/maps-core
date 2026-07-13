@file:Suppress("unused")

package io.openmobilemaps.kmp

import android.graphics.Bitmap
import android.opengl.GLES20
import com.snapchat.djinni.Future
import io.openmobilemaps.mapscore.graphics.CanvasEdgeFillMode
import io.openmobilemaps.mapscore.kmp.KMDataRef
import io.openmobilemaps.mapscore.kmp.KMFuture
import io.openmobilemaps.mapscore.kmp.KMTextureHolderInterface
import io.openmobilemaps.mapscore.kmp.KMTextureLoaderResult
import io.openmobilemaps.mapscore.kmp.asKmp as mapCoreAsKmp
import io.openmobilemaps.mapscore.shared.graphics.objects.TextureHolderInterface
import io.openmobilemaps.mapscore.shared.map.loader.TextureLoaderResult

// Shim package matching what fluid-meteogram's `kmp/api` module used to publish as
// `io.openmobilemaps:ommaps`. shv-app builds without fluid's nested openmobilemaps active
// (to avoid stale iOS API at link time), so we re-expose the same extensions here and forward
// to the canonical definitions in `io.openmobilemaps.mapscore.kmp`.

public fun java.nio.ByteBuffer.asKmp(): KMDataRef = this.mapCoreAsKmp()

public fun <T> Future<*>.asKmp(): KMFuture<T> = this.mapCoreAsKmp()

public fun TextureHolderInterface.asKmp(): KMTextureHolderInterface = this.mapCoreAsKmp()

public fun TextureLoaderResult.asKmp(): KMTextureLoaderResult = this.mapCoreAsKmp()

public fun createTextureHolder(
    data: KMDataRef,
    minFilter: Int = GLES20.GL_LINEAR,
    magFilter: Int = GLES20.GL_LINEAR,
    edgeFillMode: CanvasEdgeFillMode = CanvasEdgeFillMode.Mirorred,
): KMTextureHolderInterface? = io.openmobilemaps.mapscore.kmp.createTextureHolder(
    data = data,
    minFilter = minFilter,
    magFilter = magFilter,
    edgeFillMode = edgeFillMode,
)

public fun createTextureHolder(
    bitmap: Bitmap,
    minFilter: Int = GLES20.GL_LINEAR,
    magFilter: Int = GLES20.GL_LINEAR,
    edgeFillMode: CanvasEdgeFillMode = CanvasEdgeFillMode.Mirorred,
): KMTextureHolderInterface = io.openmobilemaps.mapscore.kmp.createTextureHolder(
    bitmap = bitmap,
    minFilter = minFilter,
    magFilter = magFilter,
    edgeFillMode = edgeFillMode,
)
