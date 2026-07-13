package io.openmobilemaps.mapscore.kmp

import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.addressOf
import kotlinx.cinterop.usePinned
import platform.Foundation.NSData
import platform.Foundation.dataWithBytes

@OptIn(ExperimentalForeignApi::class)
public actual fun ByteArray.toKMDataRef(): KMDataRef {
    if (isEmpty()) {
        return NSData.dataWithBytes(bytes = null, length = 0u).asKmp()
    }

    return usePinned { pinned ->
        NSData.dataWithBytes(
            bytes = pinned.addressOf(0),
            length = size.toULong(),
        ).asKmp()
    }
}
