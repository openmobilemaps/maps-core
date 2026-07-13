package io.openmobilemaps.mapscore.kmp

public interface MapViewOwner {
	public val mapConfig: KMMapConfig

	public fun initialize(mapInterface: KMMapInterface)

	public fun dispose()
}
