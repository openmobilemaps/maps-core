package io.openmobilemaps.mapscore.kmp

actual typealias KMDataRef = java.nio.ByteBuffer

internal fun KMDataRef.asPlatform(): java.nio.ByteBuffer = this
internal fun java.nio.ByteBuffer.asKmp(): KMDataRef = this
