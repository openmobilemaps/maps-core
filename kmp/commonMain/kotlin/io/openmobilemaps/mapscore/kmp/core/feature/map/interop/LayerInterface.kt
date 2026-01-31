package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect open class LayerInterface constructor(nativeHandle: Any? = null) {
    protected val nativeHandle: Any?

    fun setMaskingObject(maskingObject: MaskingObjectInterface?)
    fun update()
    fun buildRenderPasses(): List<RenderPassInterface>
    fun buildComputePasses(): List<ComputePassInterface>
    fun onAdded(mapInterface: MapInterface, layerIndex: Int)
    fun onRemoved()
    fun pause()
    fun resume()
    fun hide()
    fun show()

    fun setAlpha(alpha: Float)
    fun getAlpha(): Float

    fun setScissorRect(scissorRect: RectI?)

    fun isReadyToRenderOffscreen(): LayerReadyState
    fun enableAnimations(enabled: Boolean)

    fun setErrorManager(errorManager: ErrorManager)
    fun forceReload()

    fun setPrimaryRenderTarget(target: RenderTargetInterface?)
}
