package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect class Coord(
	systemIdentifier: Int,
	x: Double,
	y: Double,
	z: Double,
) {
	val systemIdentifier: Int
	val x: Double
	val y: Double
	val z: Double
}
