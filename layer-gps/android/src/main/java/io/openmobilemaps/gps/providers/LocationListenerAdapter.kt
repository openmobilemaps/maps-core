package io.openmobilemaps.gps.providers

import android.location.Location
import android.location.LocationListener
import android.os.Bundle

internal abstract class LocationListenerAdapter : LocationListener {
	abstract override fun onLocationChanged(location: Location)
	override fun onStatusChanged(provider: String?, status: Int, extras: Bundle?) = Unit
}