package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.shared.map.Camera3dConfig as MapscoreCamera3dConfig
import io.openmobilemaps.mapscore.shared.map.CameraInterpolation as MapscoreCameraInterpolation
import io.openmobilemaps.mapscore.shared.map.CameraInterpolationValue as MapscoreCameraInterpolationValue

actual class CameraInterpolationValue actual constructor(
	stop: Float,
	value: Float,
) {
	actual val stop: Float = stop
	actual val value: Float = value
}

actual class CameraInterpolation actual constructor(
	stops: List<CameraInterpolationValue>,
) {
	actual val stops: List<CameraInterpolationValue> = stops
}

actual class Camera3dConfig actual constructor(
	key: String,
	allowUserInteraction: Boolean,
	rotationSpeed: Float?,
	animationDurationMs: Int,
	minZoom: Float,
	maxZoom: Float,
	pitchInterpolationValues: CameraInterpolation,
	verticalDisplacementInterpolationValues: CameraInterpolation,
) {
	actual val key: String = key
	actual val allowUserInteraction: Boolean = allowUserInteraction
	actual val rotationSpeed: Float? = rotationSpeed
	actual val animationDurationMs: Int = animationDurationMs
	actual val minZoom: Float = minZoom
	actual val maxZoom: Float = maxZoom
	actual val pitchInterpolationValues: CameraInterpolation = pitchInterpolationValues
	actual val verticalDisplacementInterpolationValues: CameraInterpolation = verticalDisplacementInterpolationValues
}

internal fun CameraInterpolationValue.asMapscore(): MapscoreCameraInterpolationValue =
	MapscoreCameraInterpolationValue(stop = stop, value = value)

internal fun CameraInterpolation.asMapscore(): MapscoreCameraInterpolation =
	MapscoreCameraInterpolation(ArrayList(stops.map { it.asMapscore() }))

internal fun CameraInterpolationValueFromMapscore(value: MapscoreCameraInterpolationValue): CameraInterpolationValue =
	CameraInterpolationValue(stop = value.stop, value = value.value)

internal fun CameraInterpolationFromMapscore(value: MapscoreCameraInterpolation): CameraInterpolation =
	CameraInterpolation(stops = value.stops.map { CameraInterpolationValueFromMapscore(it) })

internal fun Camera3dConfig.asMapscore(): MapscoreCamera3dConfig =
	MapscoreCamera3dConfig(
		key = key,
		allowUserInteraction = allowUserInteraction,
		rotationSpeed = rotationSpeed,
		animationDurationMs = animationDurationMs,
		minZoom = minZoom,
		maxZoom = maxZoom,
		pitchInterpolationValues = pitchInterpolationValues.asMapscore(),
		verticalDisplacementInterpolationValues = verticalDisplacementInterpolationValues.asMapscore(),
	)

internal fun Camera3dConfigFromMapscore(value: MapscoreCamera3dConfig): Camera3dConfig =
	Camera3dConfig(
		key = value.key,
		allowUserInteraction = value.allowUserInteraction,
		rotationSpeed = value.rotationSpeed,
		animationDurationMs = value.animationDurationMs,
		minZoom = value.minZoom,
		maxZoom = value.maxZoom,
		pitchInterpolationValues = CameraInterpolationFromMapscore(value.pitchInterpolationValues),
		verticalDisplacementInterpolationValues = CameraInterpolationFromMapscore(value.verticalDisplacementInterpolationValues),
	)
