package io.openmobilemaps.gps

import io.openmobilemaps.mapscore.MapsCore

object LayerGps {
	fun initialize() {
		System.loadLibrary("layergps")
		MapsCore.initialize()
	}
}