package io.openmobilemaps.mapscore.kmp

import java.nio.ByteBuffer

public actual fun ByteArray.toKMDataRef(): KMDataRef {
    val buffer = ByteBuffer.allocateDirect(size)
    buffer.put(this)
    buffer.rewind()
    return buffer
}
