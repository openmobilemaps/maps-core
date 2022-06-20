package io.openmobilemaps.mapscore.map.util

import android.graphics.Bitmap

interface SaveFrameCallback {
	fun onResultByteArray(bytes: ByteArray) {}
	fun onResultBitmap(bitmap: Bitmap) {}
	fun onError()
}