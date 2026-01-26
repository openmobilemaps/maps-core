package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.kmp.feature.map.interop.MapInterface

expect class PlatformMapView

expect class MapViewWrapper() {
    val view: PlatformMapView
    val mapInterface: MapInterface
}
