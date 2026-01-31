package io.openmobilemaps.mapscore.kmp.feature.map.interop

interface MapCameraListenerInterface {
    fun onVisibleBoundsChanged(visibleBounds: RectCoord, zoom: Double)
    fun onRotationChanged(angle: Float)
    fun onMapInteraction()
    fun onCameraChange(
        viewMatrix: List<Float>,
        projectionMatrix: List<Float>,
        origin: Vec3D,
        verticalFov: Float,
        horizontalFov: Float,
        width: Float,
        height: Float,
        focusPointAltitude: Float,
        focusPointPosition: Coord,
        zoom: Float,
    )
}
