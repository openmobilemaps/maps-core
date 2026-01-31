package io.openmobilemaps.mapscore.kmp.feature.map.interop

import MapCoreSharedModule.MCMapCallbackInterfaceProtocol
import MapCoreSharedModule.MCMapInterface
import MapCoreSharedModule.MCMapReadyCallbackInterfaceProtocol
import MapCoreSharedModule.MCLayerReadyState
import MapCoreSharedModule.MCLayerReadyStateERROR
import MapCoreSharedModule.MCLayerReadyStateNOT_READY
import MapCoreSharedModule.MCLayerReadyStateREADY
import MapCoreSharedModule.MCLayerReadyStateTIMEOUT_ERROR
import platform.darwin.NSObject

actual open class MapInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
	private val map = nativeHandle as? MCMapInterface

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
			val created = MCMapInterface.create(
				graphicsFactory.asMapCore(),
				shaderFactory.asMapCore(),
				renderingContext.asMapCore(),
				mapConfig,
				scheduler.asMapCore(),
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
			val created = MCMapInterface.createWithOpenGl(
				mapConfig,
				scheduler.asMapCore(),
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
		map?.setCamera(camera.asMapCore())
	}

	actual fun getCamera(): MapCameraInterface = MapCameraInterface(map?.getCamera())

	actual fun setTouchHandler(touchHandler: TouchHandlerInterface) {
		map?.setTouchHandler(touchHandler.asMapCore())
	}

	actual fun getTouchHandler(): TouchHandlerInterface =
		TouchHandlerInterface(map?.getTouchHandler())

	actual fun setPerformanceLoggers(performanceLoggers: List<PerformanceLoggerInterface>) {
		map?.setPerformanceLoggers(performanceLoggers.mapNotNull { it.asMapCore() })
	}

	actual fun getPerformanceLoggers(): List<PerformanceLoggerInterface> =
		map?.getPerformanceLoggers()?.map { PerformanceLoggerInterface(it) } ?: emptyList()

	actual fun getLayers(): List<LayerInterface> =
		map?.getLayers()?.map { LayerInterface(it) } ?: emptyList()

	actual fun getLayersIndexed(): List<IndexedLayerInterface> =
		map?.getLayersIndexed()?.map { IndexedLayerInterface(it) } ?: emptyList()

	actual fun addLayer(layer: LayerInterface) {
		map?.addLayer(layer.asMapCore())
	}

	actual fun insertLayerAt(layer: LayerInterface, atIndex: Int) {
		map?.insertLayerAt(layer.asMapCore(), atIndex)
	}

	actual fun insertLayerAbove(layer: LayerInterface, above: LayerInterface) {
		map?.insertLayerAbove(layer.asMapCore(), above.asMapCore())
	}

	actual fun insertLayerBelow(layer: LayerInterface, below: LayerInterface) {
		map?.insertLayerBelow(layer.asMapCore(), below.asMapCore())
	}

	actual fun removeLayer(layer: LayerInterface) {
		map?.removeLayer(layer.asMapCore())
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
		map?.drawOffscreenFrame(target.asMapCore())
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

internal fun MapInterface.asMapCore(): MCMapInterface? =
	nativeHandle as? MCMapInterface

private class MapCallbackInterfaceProxy(
	private val handler: MapCallbackInterface,
) : NSObject(), MCMapCallbackInterfaceProtocol {
	override fun invalidate() {
		handler.invalidate()
	}

	override fun onMapResumed() {
		handler.onMapResumed()
	}
}

private class MapReadyCallbackInterfaceProxy(
	private val handler: MapReadyCallbackInterface,
) : NSObject(), MCMapReadyCallbackInterfaceProtocol {
	override fun stateDidUpdate(state: MCLayerReadyState) {
		handler.stateDidUpdate(state.asShared())
	}
}

private fun MCLayerReadyState.asShared(): LayerReadyState = when (this) {
	MCLayerReadyStateREADY -> LayerReadyState.READY
	MCLayerReadyStateNOT_READY -> LayerReadyState.NOT_READY
	MCLayerReadyStateERROR -> LayerReadyState.ERROR
	MCLayerReadyStateTIMEOUT_ERROR -> LayerReadyState.TIMEOUT_ERROR
	else -> LayerReadyState.NOT_READY
}
