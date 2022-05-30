package io.openmobilemaps.mapscore.map.util

import android.graphics.Bitmap
import android.graphics.Matrix
import android.opengl.GLES20
import android.util.Log
import io.openmobilemaps.mapscore.shared.graphics.common.Vec2I
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.IntBuffer
import java.nio.ShortBuffer

object SaveFrameUtil {

	fun saveCurrentFrame(screenSizePx: Vec2I, spec: SaveFrameSpec, callback: SaveFrameCallback) {
		try {
			val left = (spec.cropLeftPc * screenSizePx.x).toInt()
			val right = ((1f - spec.cropRightPc) * screenSizePx.x).toInt()
			val top = ((1f - spec.cropTopPc) * screenSizePx.y).toInt()
			val bottom = (spec.cropBottomPc * screenSizePx.y).toInt()

			val width: Int = right - left
			val height: Int = top - bottom
			val screenshotSize = width * height
			val bb = ByteBuffer.allocateDirect(screenshotSize * 4)
			bb.order(ByteOrder.nativeOrder())
			GLES20.glReadPixels(left, bottom, width, height, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, bb)

			val bitmap: Bitmap =
				when (spec.pixelFormat) {
					SaveFrameSpec.PixelFormat.RGB_565 -> {
						val pixelsBuffer = IntArray(screenshotSize)
						bb.asIntBuffer()[pixelsBuffer]
						val bitmap565 = Bitmap.createBitmap(width, height, Bitmap.Config.RGB_565)
						bitmap565.setPixels(pixelsBuffer, screenshotSize - width, -width, 0, 0, width, height)

						val sBuffer = ShortArray(screenshotSize)
						val sb = ShortBuffer.wrap(sBuffer)
						bitmap565.copyPixelsToBuffer(sb)

						for (i in 0 until screenshotSize) {
							val v: Long = sBuffer[i].toLong()
							sBuffer[i] = (((v and 0x1f) shl 11) or (v and 0x7e0) or ((v and 0xf800) shr 11)).toShort()
						}
						sb.rewind()
						bitmap565.copyPixelsFromBuffer(sb)

						bitmap565
					}
					SaveFrameSpec.PixelFormat.ARGB_8888 -> {
						var bitmap8888 = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
						bitmap8888.copyPixelsFromBuffer(bb.asIntBuffer())
						val matrix = Matrix()
						matrix.postScale(1f, -1f, width / 2f, height / 2f)
						bitmap8888 = Bitmap.createBitmap(bitmap8888, 0, 0, width, height, matrix, false)
						bitmap8888
					}
				}

			val scaledBitmap = Bitmap.createScaledBitmap(bitmap, spec.targetWidthPx ?: width, spec.targetHeightPx ?: height, true)

			when (spec.outputFormat) {
				SaveFrameSpec.OutputFormat.BITMAP -> {
					callback.onResultBitmap(bitmap)
					return
				}
				SaveFrameSpec.OutputFormat.BYTE_ARRAY -> {
					val size: Int = scaledBitmap.rowBytes * scaledBitmap.height
					val b = ByteBuffer.allocate(size)
					scaledBitmap.copyPixelsToBuffer(b)
					b.rewind()
					val bytes = ByteArray(size)
					b.get(bytes)
					callback.onResultByteArray(bytes)
					return
				}
				else -> {
					Log.e(SaveFrameUtil.javaClass.canonicalName, "Unhandled outputFormat type for saving frame: ${spec.outputFormat}")
					callback.onError()
				}
			}
		} catch (e: Exception) {
			callback.onError()
		}
	}

}