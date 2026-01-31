package io.openmobilemaps.mapscore.kmp.feature.map.interop

import MapCoreSharedModule.MCMapConfig as MapCoreMapConfig
import MapCoreSharedModule.MCMapCoordinateSystem as MapCoreMapCoordinateSystem
import MapCoreSharedModule.MCVec2F as MapCoreVec2F
import MapCoreSharedModule.MCVec2I as MapCoreVec2I
import MapCoreSharedModule.MCVec3D as MapCoreVec3D
import MapCoreSharedModule.MCRectI as MapCoreRectI
import MapCoreSharedModule.MCTiled2dMapZoomInfo as MapCoreTiled2dMapZoomInfo

actual typealias Vec2F = MapCoreVec2F
actual typealias Vec2I = MapCoreVec2I
actual typealias Vec3D = MapCoreVec3D
actual typealias RectI = MapCoreRectI
actual typealias MapConfig = MapCoreMapConfig
actual typealias MapCoordinateSystem = MapCoreMapCoordinateSystem
actual typealias Tiled2dMapZoomInfo = MapCoreTiled2dMapZoomInfo
