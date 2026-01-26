package io.openmobilemaps.mapscore.kmp.feature.map.interop

interface MapDataProviderProtocol {
	fun getStyleJson(): String?
	suspend fun loadGeojson(resourcePath: String, url: String): ByteArray?
	suspend fun loadSpriteAsync(resourcePath: String, url: String, scale: Int): ByteArray?
	suspend fun loadSpriteJsonAsync(resourcePath: String, url: String, scale: Int): ByteArray?
}
