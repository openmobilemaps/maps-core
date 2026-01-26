package io.openmobilemaps.mapscore.kmp.feature.map.interop

import android.view.View
import io.openmobilemaps.mapscore.kmp.feature.map.interop.MapInterface

actual typealias PlatformMapView = View

actual class MapViewWrapper actual constructor() {
    private lateinit var viewInternal: View
    private lateinit var mapInterfaceInternal: MapInterface

    actual val view: View
        get() = viewInternal

    actual val mapInterface: MapInterface
        get() = mapInterfaceInternal

    constructor(view: View, mapInterface: MapInterface) : this() {
        viewInternal = view
        mapInterfaceInternal = mapInterface
    }
}
