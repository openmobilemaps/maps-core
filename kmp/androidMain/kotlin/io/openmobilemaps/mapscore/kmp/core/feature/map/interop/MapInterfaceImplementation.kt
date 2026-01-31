package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.shared.map.MapInterface as MapscoreMapInterface
import io.openmobilemaps.mapscore.shared.map.MapReadyCallbackInterface as MapscoreMapReadyCallbackInterface
import io.openmobilemaps.mapscore.shared.map.MapCallbackInterface as MapscoreMapCallbackInterface

actual open class MapInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
	private val map = nativeHandle as? MapscoreMapInterface

	actual companion object {
		actual fun create(
			graphicsFactory: GraphicsObjectFactoryInterface,
			shaderFactory: ShaderFactoryInterface,
			renderingContext: RenderingContextInterface,
			mapConfig: MapConfig,
			scheduler: SchedulerInterface,
			pixelDensity: Float,
			is3d: Boolean,
		): MapInterface {
			val created = MapscoreMapInterface.create(
				requireNotNull(graphicsFactory.asMapscore()),
				requireNotNull(shaderFactory.asMapscore()),
				requireNotNull(renderingContext.asMapscore()),
				mapConfig,
				requireNotNull(scheduler.asMapscore()),
				pixelDensity,
				is3d,
			)
			return MapInterface(created)
		}

		actual fun createWithOpenGl(
			mapConfig: MapConfig,
			scheduler: SchedulerInterface,
			pixelDensity: Float,
			is3d: Boolean,
		): MapInterface {
			val created = MapscoreMapInterface.createWithOpenGl(
				mapConfig,
				requireNotNull(scheduler.asMapscore()),
				pixelDensity,
				is3d,
			)
			return MapInterface(created)
		}
	}

	actual fun setCallbackHandler(callbackInterface: MapCallbackInterface?) {
		val proxy = callbackInterface?.let { MapCallbackInterfaceProxy(it) }
		map?.setCallbackHandler(proxy)
	}

	actual fun getGraphicsObjectFactory(): GraphicsObjectFactoryInterface =
		GraphicsObjectFactoryInterface(map?.getGraphicsObjectFactory())

	actual fun getShaderFactory(): ShaderFactoryInterface =
		ShaderFactoryInterface(map?.getShaderFactory())

	actual fun getScheduler(): SchedulerInterface =
		SchedulerInterface(map?.getScheduler())

	actual fun getRenderingContext(): RenderingContextInterface =
		RenderingContextInterface(map?.getRenderingContext())

	actual fun getMapConfig(): MapConfig = requireNotNull(map?.getMapConfig())

	actual fun getCoordinateConverterHelper(): CoordinateConversionHelperInterface =
		CoordinateConversionHelperInterface(map?.getCoordinateConverterHelper())

	actual fun setCamera(camera: MapCameraInterface) {
		map?.setCamera(requireNotNull(camera.asMapscore()))
	}

	actual fun getCamera(): MapCameraInterface = MapCameraInterface(map?.getCamera())

	actual fun setTouchHandler(touchHandler: TouchHandlerInterface) {
		map?.setTouchHandler(requireNotNull(touchHandler.asMapscore()))
	}

	actual fun getTouchHandler(): TouchHandlerInterface =
		TouchHandlerInterface(map?.getTouchHandler())

	actual fun setPerformanceLoggers(performanceLoggers: List<PerformanceLoggerInterface>) {
		val list = ArrayList(performanceLoggers.mapNotNull { it.asMapscore() })
		map?.setPerformanceLoggers(list)
	}

	actual fun getPerformanceLoggers(): List<PerformanceLoggerInterface> =
		map?.getPerformanceLoggers()?.map { PerformanceLoggerInterface(it) } ?: emptyList()

	actual fun getLayers(): List<LayerInterface> =
		map?.getLayers()?.map { LayerInterface(it) } ?: emptyList()

	actual fun getLayersIndexed(): List<IndexedLayerInterface> =
		map?.getLayersIndexed()?.map { IndexedLayerInterface(it) } ?: emptyList()

	actual fun addLayer(layer: LayerInterface) {
		map?.addLayer(requireNotNull(layer.asMapscore()))
	}

	actual fun insertLayerAt(layer: LayerInterface, atIndex: Int) {
		map?.insertLayerAt(requireNotNull(layer.asMapscore()), atIndex)
	}

	actual fun insertLayerAbove(layer: LayerInterface, above: LayerInterface) {
		map?.insertLayerAbove(requireNotNull(layer.asMapscore()), requireNotNull(above.asMapscore()))
	}

	actual fun insertLayerBelow(layer: LayerInterface, below: LayerInterface) {
		map?.insertLayerBelow(requireNotNull(layer.asMapscore()), requireNotNull(below.asMapscore()))
	}

	actual fun removeLayer(layer: LayerInterface) {
		map?.removeLayer(requireNotNull(layer.asMapscore()))
	}

	actual fun setViewportSize(size: Vec2I) {
		map?.setViewportSize(size)
	}

	actual fun setBackgroundColor(color: Color) {
		map?.setBackgroundColor(color)
	}

	actual fun is3d(): Boolean = map?.is3d() ?: false

	actual fun invalidate() {
		map?.invalidate()
	}

	actual fun resetIsInvalidated() {
		map?.resetIsInvalidated()
	}

	actual fun prepare() {
		map?.prepare()
	}

	actual fun getNeedsCompute(): Boolean = map?.getNeedsCompute() ?: false

	actual fun drawOffscreenFrame(target: RenderTargetInterface) {
		map?.drawOffscreenFrame(requireNotNull(target.asMapscore()))
	}

	actual fun drawFrame() {
		map?.drawFrame()
	}

	actual fun compute() {
		map?.compute()
	}

	actual fun resume() {
		map?.resume()
	}

	actual fun pause() {
		map?.pause()
	}

	actual fun destroy() {
		map?.destroy()
	}

	actual fun drawReadyFrame(bounds: RectCoord, paddingPc: Float, timeout: Float, callbacks: MapReadyCallbackInterface) {
		val proxy = MapReadyCallbackInterfaceProxy(callbacks)
		map?.drawReadyFrame(bounds, paddingPc, timeout, proxy)
	}

	actual fun forceReload() {
		map?.forceReload()
	}
}

internal fun MapInterface.asMapscore(): MapscoreMapInterface? =
	nativeHandle as? MapscoreMapInterface

private class MapCallbackInterfaceProxy(
	private val handler: MapCallbackInterface,
) : MapscoreMapCallbackInterface() {
	override fun invalidate() {
		handler.invalidate()
	}

	override fun onMapResumed() {
		handler.onMapResumed()
	}
}

private class MapReadyCallbackInterfaceProxy(
	private val handler: MapReadyCallbackInterface,
) : MapscoreMapReadyCallbackInterface() {
	override fun stateDidUpdate(state: io.openmobilemaps.mapscore.shared.map.LayerReadyState) {
		handler.stateDidUpdate(state.asShared())
	}
}
