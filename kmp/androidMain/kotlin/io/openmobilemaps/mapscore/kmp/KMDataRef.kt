package io.openmobilemaps.mapscore.kmp

actual typealias KMDataRef = java.nio.ByteBuffer

public fun KMDataRef.asPlatform(): java.nio.ByteBuffer = this
public fun java.nio.ByteBuffer.asKmp(): KMDataRef = this
