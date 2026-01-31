package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.shared.map.MapCamera3dInterface as MapscoreMapCamera3dInterface
import io.openmobilemaps.mapscore.shared.map.MapCameraInterface as MapscoreMapCameraInterface
import io.openmobilemaps.mapscore.shared.map.camera.MapCameraListenerInterface as MapscoreMapCameraListenerInterface

actual open class MapCameraInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
	private val camera = nativeHandle as? MapscoreMapCameraInterface

	actual companion object {
		actual fun create(mapInterface: MapInterface, screenDensityPpi: Float, is3d: Boolean): MapCameraInterface {
			val created = MapscoreMapCameraInterface.create(requireNotNull(mapInterface.asMapscore()), screenDensityPpi, is3d)
			return MapCameraInterface(created)
		}
	}

	actual fun freeze(freeze: Boolean) {
		camera?.freeze(freeze)
	}

	actual fun moveToCenterPositionZoom(centerPosition: Coord, zoom: Double, animated: Boolean) {
		camera?.moveToCenterPositionZoom(centerPosition, zoom, animated)
	}

	actual fun moveToCenterPosition(centerPosition: Coord, animated: Boolean) {
		camera?.moveToCenterPosition(centerPosition, animated)
	}

	actual fun moveToBoundingBox(boundingBox: RectCoord, paddingPc: Float, animated: Boolean, minZoom: Double?, maxZoom: Double?) {
		camera?.moveToBoundingBox(boundingBox, paddingPc, animated, minZoom, maxZoom)
	}

	actual fun getCenterPosition(): Coord = requireNotNull(camera?.getCenterPosition())

	actual fun setZoom(zoom: Double, animated: Boolean) {
		camera?.setZoom(zoom, animated)
	}

	actual fun getZoom(): Double = requireNotNull(camera?.getZoom())

	actual fun setRotation(angle: Float, animated: Boolean) {
		camera?.setRotation(angle, animated)
	}

	actual fun getRotation(): Float = requireNotNull(camera?.getRotation())

	actual fun setMinZoom(minZoom: Double) {
		camera?.setMinZoom(minZoom)
	}

	actual fun setMaxZoom(maxZoom: Double) {
		camera?.setMaxZoom(maxZoom)
	}

	actual fun getMinZoom(): Double = requireNotNull(camera?.getMinZoom())

	actual fun getMaxZoom(): Double = requireNotNull(camera?.getMaxZoom())

	actual fun setBounds(bounds: RectCoord) {
		camera?.setBounds(bounds)
	}

	actual fun getBounds(): RectCoord = requireNotNull(camera?.getBounds())

	actual fun isInBounds(coords: Coord): Boolean = camera?.isInBounds(coords) ?: false

	actual fun setPaddingLeft(padding: Float) {
		camera?.setPaddingLeft(padding)
	}

	actual fun setPaddingRight(padding: Float) {
		camera?.setPaddingRight(padding)
	}

	actual fun setPaddingTop(padding: Float) {
		camera?.setPaddingTop(padding)
	}

	actual fun setPaddingBottom(padding: Float) {
		camera?.setPaddingBottom(padding)
	}

	actual fun getVisibleRect(): RectCoord = requireNotNull(camera?.getVisibleRect())

	actual fun getPaddingAdjustedVisibleRect(): RectCoord = requireNotNull(camera?.getPaddingAdjustedVisibleRect())

	actual fun getScreenDensityPpi(): Float = requireNotNull(camera?.getScreenDensityPpi())

	actual fun update() {
		camera?.update()
	}

	actual fun getInvariantModelMatrix(coordinate: Coord, scaleInvariant: Boolean, rotationInvariant: Boolean): List<Float> =
		camera?.getInvariantModelMatrix(coordinate, scaleInvariant, rotationInvariant) ?: emptyList()

	actual fun addListener(listener: MapCameraListenerInterface) {
		camera?.addListener(MapCameraListenerProxy(listener))
	}

	actual fun removeListener(listener: MapCameraListenerInterface) {
		camera?.removeListener(MapCameraListenerProxy(listener))
	}

	actual fun notifyListenerBoundsChange() {
		camera?.notifyListenerBoundsChange()
	}

	actual fun coordFromScreenPosition(posScreen: Vec2F): Coord =
		requireNotNull(camera?.coordFromScreenPosition(posScreen))

	actual fun coordFromScreenPositionZoom(posScreen: Vec2F, zoom: Float): Coord =
		requireNotNull(camera?.coordFromScreenPositionZoom(posScreen, zoom))

	actual fun screenPosFromCoord(coord: Coord): Vec2F =
		requireNotNull(camera?.screenPosFromCoord(coord))

	actual fun screenPosFromCoordZoom(coord: Coord, zoom: Float): Vec2F =
		requireNotNull(camera?.screenPosFromCoordZoom(coord, zoom))

	actual fun mapUnitsFromPixels(distancePx: Double): Double = requireNotNull(camera?.mapUnitsFromPixels(distancePx))

	actual fun getScalingFactor(): Double = requireNotNull(camera?.getScalingFactor())

	actual fun coordIsVisibleOnScreen(coord: Coord, paddingPc: Float): Boolean =
		camera?.coordIsVisibleOnScreen(coord, paddingPc) ?: false

	actual fun setRotationEnabled(enabled: Boolean) {
		camera?.setRotationEnabled(enabled)
	}

	actual fun setSnapToNorthEnabled(enabled: Boolean) {
		camera?.setSnapToNorthEnabled(enabled)
	}

	actual fun setBoundsRestrictWholeVisibleRect(enabled: Boolean) {
		camera?.setBoundsRestrictWholeVisibleRect(enabled)
	}

	actual fun asCameraInterface(): CameraInterface = CameraInterface(requireNotNull(camera?.asCameraInterface()))

	actual fun getLastVpMatrixD(): List<Double>? = camera?.getLastVpMatrixD()

	actual fun getLastVpMatrix(): List<Float>? = camera?.getLastVpMatrix()

	actual fun getLastInverseVpMatrix(): List<Float>? = camera?.getLastInverseVpMatrix()

	actual fun getLastVpMatrixViewBounds(): RectCoord? = camera?.getLastVpMatrixViewBounds()

	actual fun getLastVpMatrixRotation(): Float? = camera?.getLastVpMatrixRotation()

	actual fun getLastVpMatrixZoom(): Float? = camera?.getLastVpMatrixZoom()

	actual fun getLastCameraPosition(): Vec3D? = camera?.getLastCameraPosition()

	actual fun asMapCamera3d(): MapCamera3dInterface? = camera?.asMapCamera3d()?.let { MapCamera3dInterface(it) }
}

internal fun MapCameraInterface.asMapscore(): MapscoreMapCameraInterface? =
	nativeHandle as? MapscoreMapCameraInterface

private class MapCameraListenerProxy(
	private val handler: MapCameraListenerInterface,
) : MapscoreMapCameraListenerInterface() {
	override fun onVisibleBoundsChanged(visibleBounds: RectCoord, zoom: Double) {
		handler.onVisibleBoundsChanged(visibleBounds, zoom)
	}

	override fun onRotationChanged(angle: Float) {
		handler.onRotationChanged(angle)
	}

	override fun onMapInteraction() {
		handler.onMapInteraction()
	}

	override fun onCameraChange(
		viewMatrix: ArrayList<Float>,
		projectionMatrix: ArrayList<Float>,
		origin: Vec3D,
		verticalFov: Float,
		horizontalFov: Float,
		width: Float,
		height: Float,
		focusPointAltitude: Float,
		focusPointPosition: Coord,
		zoom: Float,
	) {
		handler.onCameraChange(
			viewMatrix = viewMatrix,
			projectionMatrix = projectionMatrix,
			origin = origin,
			verticalFov = verticalFov,
			horizontalFov = horizontalFov,
			width = width,
			height = height,
			focusPointAltitude = focusPointAltitude,
			focusPointPosition = focusPointPosition,
			zoom = zoom,
		)
	}
}

actual open class MapCamera3dInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
	private val camera3d = nativeHandle as? MapscoreMapCamera3dInterface

	actual fun getCameraConfig(): Camera3dConfig = requireNotNull(camera3d?.getCameraConfig())

	actual fun setCameraConfig(
		config: Camera3dConfig,
		durationSeconds: Float?,
		targetZoom: Float?,
		targetCoordinate: Coord?,
	) {
		camera3d?.setCameraConfig(config, durationSeconds, targetZoom, targetCoordinate)
	}
}

actual class Camera3dConfigFactory actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	actual companion object {
		actual fun getBasicConfig(): Camera3dConfig =
			io.openmobilemaps.mapscore.shared.map.Camera3dConfigFactory.getBasicConfig()

		actual fun getRestorConfig(): Camera3dConfig =
			io.openmobilemaps.mapscore.shared.map.Camera3dConfigFactory.getRestorConfig()
	}
}
