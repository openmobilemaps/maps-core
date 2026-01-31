package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect class MapCoordinateSystem(
    identifier: Int,
    bounds: RectCoord,
    unitToScreenMeterFactor: Float,
) {
    val identifier: Int
    val bounds: RectCoord
    val unitToScreenMeterFactor: Float
}
