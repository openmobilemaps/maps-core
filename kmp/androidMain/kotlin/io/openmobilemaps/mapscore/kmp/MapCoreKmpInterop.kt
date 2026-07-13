package io.openmobilemaps.mapscore.kmp

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.ImageDecoder
import android.opengl.GLES20
import android.os.Build
import android.util.Log
import io.openmobilemaps.mapscore.graphics.BitmapTextureHolder
import io.openmobilemaps.mapscore.graphics.CanvasEdgeFillMode
import java.io.IOException
import java.nio.ByteBuffer

fun createTextureHolder(
    data: KMDataRef,
    minFilter: Int = GLES20.GL_LINEAR,
    magFilter: Int = GLES20.GL_LINEAR,
    edgeFillMode: CanvasEdgeFillMode = CanvasEdgeFillMode.Mirorred,
): KMTextureHolderInterface? {
    val bytes = data.toByteArray()

    val bitmap = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
        try {
            ImageDecoder.decodeBitmap(ImageDecoder.createSource(bytes)) { decoder, info, source ->
                decoder.apply {
                    allocator = ImageDecoder.ALLOCATOR_SOFTWARE
                    isMutableRequired = true
                }
            }
        } catch (e: IOException) {
            Log.e(KMTextureHolderInterface::class.java.canonicalName, "Failed to decode image $data", e)
            null
        }
    } else {
        BitmapFactory.decodeByteArray(bytes, 0, bytes.size)
    }

    return bitmap?.let { BitmapTextureHolder(it, minFilter, magFilter, edgeFillMode) }?.asKmp()
}

fun createTextureHolder(
    bitmap: Bitmap,
    minFilter: Int = GLES20.GL_LINEAR,
    magFilter: Int = GLES20.GL_LINEAR,
    edgeFillMode: CanvasEdgeFillMode = CanvasEdgeFillMode.Mirorred,
): KMTextureHolderInterface = BitmapTextureHolder(bitmap, minFilter, magFilter, edgeFillMode).asKmp()


private fun ByteBuffer.toByteArray(): ByteArray {
    val duplicate = duplicate()
    duplicate.rewind()
    val bytes = ByteArray(duplicate.remaining())
    duplicate.get(bytes)
    return bytes
}
