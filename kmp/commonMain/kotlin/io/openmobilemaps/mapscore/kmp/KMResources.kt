package io.openmobilemaps.mapscore.kmp

interface KMResources {
	suspend fun readBytes(resourcePath: String) : Result<ByteArray>
}