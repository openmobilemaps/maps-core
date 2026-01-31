package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect open class MapCameraInterface constructor(nativeHandle: Any? = null) {
    protected val nativeHandle: Any?

    companion object {
        fun create(mapInterface: MapInterface, screenDensityPpi: Float, is3d: Boolean): MapCameraInterface
    }

    fun freeze(freeze: Boolean)

    fun moveToCenterPositionZoom(centerPosition: Coord, zoom: Double, animated: Boolean)
    fun moveToCenterPosition(centerPosition: Coord, animated: Boolean)
    fun moveToBoundingBox(boundingBox: RectCoord, paddingPc: Float, animated: Boolean, minZoom: Double?, maxZoom: Double?)
    fun getCenterPosition(): Coord

    fun setZoom(zoom: Double, animated: Boolean)
    fun getZoom(): Double

    fun setRotation(angle: Float, animated: Boolean)
    fun getRotation(): Float

    fun setMinZoom(minZoom: Double)
    fun setMaxZoom(maxZoom: Double)

    fun getMinZoom(): Double
    fun getMaxZoom(): Double

    fun setBounds(bounds: RectCoord)
    fun getBounds(): RectCoord
    fun isInBounds(coords: Coord): Boolean

    fun setPaddingLeft(padding: Float)
    fun setPaddingRight(padding: Float)
    fun setPaddingTop(padding: Float)
    fun setPaddingBottom(padding: Float)

    fun getVisibleRect(): RectCoord
    fun getPaddingAdjustedVisibleRect(): RectCoord

    fun getScreenDensityPpi(): Float

    fun update()

    fun getInvariantModelMatrix(coordinate: Coord, scaleInvariant: Boolean, rotationInvariant: Boolean): List<Float>

    fun addListener(listener: MapCameraListenerInterface)
    fun removeListener(listener: MapCameraListenerInterface)
    fun notifyListenerBoundsChange()

    fun coordFromScreenPosition(posScreen: Vec2F): Coord
    fun coordFromScreenPositionZoom(posScreen: Vec2F, zoom: Float): Coord

    fun screenPosFromCoord(coord: Coord): Vec2F
    fun screenPosFromCoordZoom(coord: Coord, zoom: Float): Vec2F

    fun mapUnitsFromPixels(distancePx: Double): Double
    fun getScalingFactor(): Double

    fun coordIsVisibleOnScreen(coord: Coord, paddingPc: Float): Boolean

    fun setRotationEnabled(enabled: Boolean)
    fun setSnapToNorthEnabled(enabled: Boolean)
    fun setBoundsRestrictWholeVisibleRect(enabled: Boolean)

    fun asCameraInterface(): CameraInterface

    fun getLastVpMatrixD(): List<Double>?
    fun getLastVpMatrix(): List<Float>?
    fun getLastInverseVpMatrix(): List<Float>?
    fun getLastVpMatrixViewBounds(): RectCoord?
    fun getLastVpMatrixRotation(): Float?
    fun getLastVpMatrixZoom(): Float?
    fun getLastCameraPosition(): Vec3D?

    fun asMapCamera3d(): MapCamera3dInterface?
}
