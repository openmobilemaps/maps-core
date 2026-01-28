package io.openmobilemaps.mapscore.kmp.feature.map.interop

import kotlin.experimental.ExperimentalObjCName
import kotlin.native.ObjCName

@OptIn(ExperimentalObjCName::class)
@ObjCName("MapCoreInterop", exact = true)
actual object MapCoreInterop {
	actual fun moveToCenter(coord: Coord) {
		coord.hashCode()
	}
}
