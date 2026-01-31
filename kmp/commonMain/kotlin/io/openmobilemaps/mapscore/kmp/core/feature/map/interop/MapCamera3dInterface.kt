package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect open class MapCamera3dInterface constructor(nativeHandle: Any? = null) {
    protected val nativeHandle: Any?

    fun getCameraConfig(): Camera3dConfig

    fun setCameraConfig(
        config: Camera3dConfig,
        durationSeconds: Float?,
        targetZoom: Float?,
        targetCoordinate: Coord?,
    )
}
