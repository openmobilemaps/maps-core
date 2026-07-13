package io.openmobilemaps.mapscore.kmp

import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.addressOf
import kotlinx.cinterop.usePinned
import platform.Foundation.NSData
import platform.Foundation.dataWithBytes
import swiftPMImport.io.openmobilemaps.mapscore.kmp.MCTextureHolderInterfaceProtocol
import swiftPMImport.io.openmobilemaps.mapscore.kmp.TextureHolder
internal actual fun decodeTextureHolder(imageBytes: ByteArray): KMTextureHolderInterface? {
	val data = imageBytes.toNSData()
	val holder = TextureHolder(data = data)
	return (holder as MCTextureHolderInterfaceProtocol).asKmp()
}

@OptIn(ExperimentalForeignApi::class)
private fun ByteArray.toNSData(): NSData {
	if (isEmpty()) {
		return NSData.dataWithBytes(bytes = null, length = 0u)
	}
	return usePinned { pinned ->
		NSData.dataWithBytes(
			bytes = pinned.addressOf(0),
			length = size.toULong(),
		)
	}
}
