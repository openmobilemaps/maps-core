package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect class CameraInterpolationValue(
    stop: Float,
    value: Float,
) {
    val stop: Float
    val value: Float
}

expect class CameraInterpolation(
    stops: List<CameraInterpolationValue>,
) {
    val stops: List<CameraInterpolationValue>
}

expect class Camera3dConfig(
    key: String,
    allowUserInteraction: Boolean,
    rotationSpeed: Float?,
    animationDurationMs: Int,
    minZoom: Float,
    maxZoom: Float,
    pitchInterpolationValues: CameraInterpolation,
    verticalDisplacementInterpolationValues: CameraInterpolation,
) {
    val key: String
    val allowUserInteraction: Boolean
    val rotationSpeed: Float?
    val animationDurationMs: Int
    val minZoom: Float
    val maxZoom: Float
    val pitchInterpolationValues: CameraInterpolation
    val verticalDisplacementInterpolationValues: CameraInterpolation
}

expect open class Camera3dConfigFactory constructor(nativeHandle: Any? = null) {
    protected val nativeHandle: Any?

    companion object {
        fun getBasicConfig(): Camera3dConfig
        fun getRestorConfig(): Camera3dConfig
    }
}
