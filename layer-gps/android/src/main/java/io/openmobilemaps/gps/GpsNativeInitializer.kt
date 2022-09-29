package io.openmobilemaps.gps

object GpsNativeInitializer {
	init {
		System.loadLibrary("gps")
	}
}