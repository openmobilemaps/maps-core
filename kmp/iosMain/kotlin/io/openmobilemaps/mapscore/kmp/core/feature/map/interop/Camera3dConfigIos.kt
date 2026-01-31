package io.openmobilemaps.mapscore.kmp.feature.map.interop

import MapCoreSharedModule.MCCamera3dConfig
import MapCoreSharedModule.MCCameraInterpolation
import MapCoreSharedModule.MCCameraInterpolationValue
import platform.Foundation.NSNumber

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

internal fun CameraInterpolationValue.asMapCore(): MCCameraInterpolationValue =
    MCCameraInterpolationValue.cameraInterpolationValueWithStop(stop = stop, value = value)

internal fun CameraInterpolation.asMapCore(): MCCameraInterpolation =
    MCCameraInterpolation.cameraInterpolationWithStops(stops.map { it.asMapCore() })

internal fun CameraInterpolationValueFromMapCore(value: MCCameraInterpolationValue): CameraInterpolationValue =
    CameraInterpolationValue(stop = value.stop, value = value.value)

internal fun CameraInterpolationFromMapCore(value: MCCameraInterpolation): CameraInterpolation =
    CameraInterpolation(stops = value.stops.map { CameraInterpolationValueFromMapCore(it) })

internal fun Camera3dConfig.asMapCore(): MCCamera3dConfig =
    MCCamera3dConfig.camera3dConfigWithKey(
        key = key,
        allowUserInteraction = allowUserInteraction,
        rotationSpeed = rotationSpeed?.let { NSNumber(float = it) },
        animationDurationMs = animationDurationMs,
        minZoom = minZoom,
        maxZoom = maxZoom,
        pitchInterpolationValues = pitchInterpolationValues.asMapCore(),
        verticalDisplacementInterpolationValues = verticalDisplacementInterpolationValues.asMapCore(),
    )

internal fun Camera3dConfigFromMapCore(value: MCCamera3dConfig): Camera3dConfig =
    Camera3dConfig(
        key = value.key,
        allowUserInteraction = value.allowUserInteraction,
        rotationSpeed = value.rotationSpeed?.floatValue,
        animationDurationMs = value.animationDurationMs.toInt(),
        minZoom = value.minZoom,
        maxZoom = value.maxZoom,
        pitchInterpolationValues = CameraInterpolationFromMapCore(value.pitchInterpolationValues),
        verticalDisplacementInterpolationValues = CameraInterpolationFromMapCore(value.verticalDisplacementInterpolationValues),
    )
