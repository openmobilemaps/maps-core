package io.openmobilemaps.mapscore.kmp.feature.map.interop

import LayerGpsSharedModule.MCGpsLayerCallbackInterfaceProtocol
import LayerGpsSharedModule.MCGpsLayerInterface
import LayerGpsSharedModule.MCGpsMode
import LayerGpsSharedModule.MCGpsModeDISABLED
import LayerGpsSharedModule.MCGpsModeFOLLOW
import LayerGpsSharedModule.MCGpsModeFOLLOW_AND_TURN
import LayerGpsSharedModule.MCGpsModeSTANDARD
import LayerGpsSharedModule.MCGpsStyleInfoInterface
import LayerGpsSharedModule.MCColor
import LayerGpsSharedModule.MCTextureHolderInterfaceProtocol as GpsTextureHolderInterfaceProtocol
import MapCoreObjC.MCMapCoreObjCFactory
import MapCoreSharedModule.MCCoordinateSystemIdentifiers
import io.openmobilemaps.mapscore.kmp.feature.map.model.GpsMode
import kotlinx.cinterop.useContents
import platform.CoreLocation.CLHeading
import platform.CoreLocation.CLLocation
import platform.CoreLocation.CLLocationManager
import platform.CoreLocation.CLLocationManagerDelegateProtocol
import platform.darwin.NSObject
import platform.UIKit.UIImage
import platform.UIKit.UIImagePNGRepresentation

actual abstract class MapGpsLayer actual constructor(nativeHandle: Any?) : LayerInterface(
	(nativeHandle as? MCGpsLayerInterface)?.asLayerInterface(),
) {
	actual abstract fun setMode(mode: GpsMode)
	actual abstract fun getMode(): GpsMode
	actual abstract fun setOnModeChangedListener(listener: ((GpsMode) -> Unit)?)
	actual abstract fun notifyPermissionGranted()
	actual abstract fun lastLocation(): Coord?
}

class MapGpsLayerImpl(nativeHandle: Any?) : MapGpsLayer(nativeHandle) {
	private val gpsLayer: MCGpsLayerInterface =
		requireNotNull(MCGpsLayerInterface.create(styleInfo = defaultStyle()))
	private val locationManager = CLLocationManager()
	private val locationDelegate = GpsLocationDelegate(this)
	private val callbackHandler = GpsLayerCallbackHandler(this)
	private var modeListener: ((GpsMode) -> Unit)? = null
	private var lastKnownLocation: Coord? = null

	init {
		gpsLayer.setCallbackHandler(callbackHandler)
		gpsLayer.enableHeading(true)
		locationManager.delegate = locationDelegate
		locationManager.desiredAccuracy = platform.CoreLocation.kCLLocationAccuracyBest
		locationManager.headingFilter = 1.0
	}

	override fun setMode(mode: GpsMode) {
		gpsLayer.setMode(mode.asLayerMode())
	}

	override fun getMode(): GpsMode = gpsLayer.getMode().asSharedMode()

	override fun setOnModeChangedListener(listener: ((GpsMode) -> Unit)?) {
		modeListener = listener
	}

	override fun notifyPermissionGranted() {
		locationManager.startUpdatingLocation()
		locationManager.startUpdatingHeading()
	}

	override fun lastLocation(): Coord? = lastKnownLocation

	internal fun updateLocation(location: CLLocation) {
		val coord = location.coordinate.useContents {
			GpsCoord(
				systemIdentifier = MCCoordinateSystemIdentifiers.EPSG4326(),
				x = longitude,
				y = latitude,
				z = location.altitude,
			)
		}
		gpsLayer.setDrawPoint(shouldDrawHeading())
		gpsLayer.setDrawHeading(shouldDrawHeading())
		gpsLayer.updatePosition(coord, horizontalAccuracyM = location.horizontalAccuracy)
		lastKnownLocation = Coord(
			systemIdentifier = coord.systemIdentifier(),
			x = coord.x(),
			y = coord.y(),
			z = coord.z(),
		)
	}

	internal fun updateHeading(heading: Double) {
		gpsLayer.updateHeading(heading.toFloat())
	}

	internal fun handleModeChanged(mode: MCGpsMode) {
		modeListener?.invoke(mode.asSharedMode())
	}

	private fun shouldDrawHeading(): Boolean = true
}

private class GpsLayerCallbackHandler(
	private val layer: MapGpsLayerImpl,
) : NSObject(), MCGpsLayerCallbackInterfaceProtocol {
	override fun modeDidChange(mode: MCGpsMode) {
		layer.handleModeChanged(mode)
	}

	override fun onPointClick(coordinate: GpsCoord) {
		// no-op
	}
}

private class GpsLocationDelegate(
	private val layer: MapGpsLayerImpl,
) : NSObject(), CLLocationManagerDelegateProtocol {
	override fun locationManager(manager: CLLocationManager, didUpdateLocations: List<*>) {
		val location = didUpdateLocations.lastOrNull() as? CLLocation ?: return
		layer.updateLocation(location)
	}

	override fun locationManager(manager: CLLocationManager, didUpdateHeading: CLHeading) {
		layer.updateHeading(didUpdateHeading.trueHeading)
	}

	override fun locationManager(manager: CLLocationManager, didFailWithError: platform.Foundation.NSError) {
		layer.setMode(GpsMode.DISABLED)
	}
}

private fun GpsMode.asLayerMode(): MCGpsMode = when (this) {
	GpsMode.DISABLED -> MCGpsModeDISABLED
	GpsMode.STANDARD -> MCGpsModeSTANDARD
	GpsMode.FOLLOW -> MCGpsModeFOLLOW
}

private fun MCGpsMode.asSharedMode(): GpsMode = when (this) {
	MCGpsModeDISABLED -> GpsMode.DISABLED
	MCGpsModeSTANDARD -> GpsMode.STANDARD
	MCGpsModeFOLLOW -> GpsMode.FOLLOW
	MCGpsModeFOLLOW_AND_TURN -> GpsMode.FOLLOW
	else -> GpsMode.STANDARD
}

private fun defaultStyle(): MCGpsStyleInfoInterface? {
	val pointTexture = loadTexture("ic_gps_point")
	val headingTexture = loadTexture("ic_gps_direction")
	val accuracyColor = MCColor(r = 112f / 255f, g = 173f / 255f, b = 204f / 255f, a = 0.2f)
	return MCGpsStyleInfoInterface.create(pointTexture, headingTexture = headingTexture, courseTexture = null, accuracyColor = accuracyColor)
}

private fun loadTexture(name: String): GpsTextureHolderInterfaceProtocol? {
	val bundle = findBundleWithImage(name) ?: return null
	val image = UIImage.imageNamed(name, inBundle = bundle, compatibleWithTraitCollection = null) ?: return null
	val data = UIImagePNGRepresentation(image) ?: return null
	return MCMapCoreObjCFactory.createTextureHolderWithData(data) as? GpsTextureHolderInterfaceProtocol
}

private fun findBundleWithImage(name: String): platform.Foundation.NSBundle? {
	val bundles = (platform.Foundation.NSBundle.allBundles + platform.Foundation.NSBundle.allFrameworks)
		.mapNotNull { it as? platform.Foundation.NSBundle }
	for (bundle in bundles) {
		val image = UIImage.imageNamed(name, inBundle = bundle, compatibleWithTraitCollection = null)
		if (image != null) return bundle
	}
	return null
}
