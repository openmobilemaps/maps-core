package io.openmobilemaps.mapscore.kmp.feature.map.interop

import kotlin.experimental.ExperimentalObjCName
import kotlin.native.ObjCName

import MapCoreObjC.MCMapViewObjC
import MapCoreSharedModule.MCMapInterface
import platform.UIKit.UIView

actual typealias PlatformMapView = UIView

@OptIn(ExperimentalObjCName::class)
@ObjCName("MapViewWrapper", exact = true)
actual class MapViewWrapper actual constructor() {
    private val mapView = MCMapViewObjC()
    @Suppress("CAST_NEVER_SUCCEEDS")
    private val mapInterfaceImplementation = MapInterface(mapView.mapInterface as MCMapInterface)

    actual val view: UIView = mapView
    actual val mapInterface: MapInterface = mapInterfaceImplementation
}
