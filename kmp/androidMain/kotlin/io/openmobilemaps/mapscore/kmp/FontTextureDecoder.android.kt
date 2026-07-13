package io.openmobilemaps.mapscore.kmp

import java.nio.ByteBuffer

internal actual fun decodeTextureHolder(imageBytes: ByteArray): KMTextureHolderInterface? {
	val buffer = ByteBuffer.allocateDirect(imageBytes.size)
	buffer.put(imageBytes)
	buffer.rewind()
	return createTextureHolder(buffer)
}
