package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect class RectCoord(
	topLeft: Coord,
	bottomRight: Coord,
) {
	val topLeft: Coord
	val bottomRight: Coord
}
