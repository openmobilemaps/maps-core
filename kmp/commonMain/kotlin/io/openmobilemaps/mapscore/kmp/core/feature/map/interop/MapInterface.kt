package io.openmobilemaps.mapscore.kmp.feature.map.interop

expect open class MapInterface constructor(nativeHandle: Any? = null) {
    protected val nativeHandle: Any?

    companion object {
        fun create(
            graphicsFactory: GraphicsObjectFactoryInterface,
            shaderFactory: ShaderFactoryInterface,
            renderingContext: RenderingContextInterface,
            mapConfig: MapConfig,
            scheduler: SchedulerInterface,
            pixelDensity: Float,
            is3d: Boolean,
        ): MapInterface

        fun createWithOpenGl(
            mapConfig: MapConfig,
            scheduler: SchedulerInterface,
            pixelDensity: Float,
            is3d: Boolean,
        ): MapInterface
    }

    fun setCallbackHandler(callbackInterface: MapCallbackInterface?)

    fun getGraphicsObjectFactory(): GraphicsObjectFactoryInterface
    fun getShaderFactory(): ShaderFactoryInterface
    fun getScheduler(): SchedulerInterface
    fun getRenderingContext(): RenderingContextInterface
    fun getMapConfig(): MapConfig
    fun getCoordinateConverterHelper(): CoordinateConversionHelperInterface

    fun setCamera(camera: MapCameraInterface)
    fun getCamera(): MapCameraInterface

    fun setTouchHandler(touchHandler: TouchHandlerInterface)
    fun getTouchHandler(): TouchHandlerInterface

    fun setPerformanceLoggers(performanceLoggers: List<PerformanceLoggerInterface>)
    fun getPerformanceLoggers(): List<PerformanceLoggerInterface>

    fun getLayers(): List<LayerInterface>
    fun getLayersIndexed(): List<IndexedLayerInterface>

    fun addLayer(layer: LayerInterface)
    fun insertLayerAt(layer: LayerInterface, atIndex: Int)
    fun insertLayerAbove(layer: LayerInterface, above: LayerInterface)
    fun insertLayerBelow(layer: LayerInterface, below: LayerInterface)

    fun removeLayer(layer: LayerInterface)

    fun setViewportSize(size: Vec2I)
    fun setBackgroundColor(color: Color)

    fun is3d(): Boolean

    fun invalidate()
    fun resetIsInvalidated()
    fun prepare()
    fun getNeedsCompute(): Boolean

    fun drawOffscreenFrame(target: RenderTargetInterface)
    fun drawFrame()
    fun compute()
    fun resume()
    fun pause()
    fun destroy()

    fun drawReadyFrame(bounds: RectCoord, paddingPc: Float, timeout: Float, callbacks: MapReadyCallbackInterface)
    fun forceReload()
}
