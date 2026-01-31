package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.shared.map.LayerInterface as MapscoreLayerInterface
import io.openmobilemaps.mapscore.shared.map.LayerReadyState as MapscoreLayerReadyState

actual open class LayerInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	private val layer = nativeHandle as? MapscoreLayerInterface

	internal fun asMapscore(): MapscoreLayerInterface? =
		nativeHandle as? MapscoreLayerInterface

	actual fun setMaskingObject(maskingObject: MaskingObjectInterface?) {
		layer?.setMaskingObject(maskingObject?.asMapscore())
	}

	actual fun update() {
		layer?.update()
	}

	actual fun buildRenderPasses(): List<RenderPassInterface> {
		return layer?.buildRenderPasses()?.map { RenderPassInterface(it) } ?: emptyList()
	}

	actual fun buildComputePasses(): List<ComputePassInterface> {
		return layer?.buildComputePasses()?.map { ComputePassInterface(it) } ?: emptyList()
	}

	actual fun onAdded(mapInterface: MapInterface, layerIndex: Int) {
		layer?.onAdded(mapInterface.asMapscore(), layerIndex)
	}

	actual fun onRemoved() {
		layer?.onRemoved()
	}

	actual fun pause() {
		layer?.pause()
	}

	actual fun resume() {
		layer?.resume()
	}

	actual fun hide() {
		layer?.hide()
	}

	actual fun show() {
		layer?.show()
	}

	actual fun setAlpha(alpha: Float) {
		layer?.setAlpha(alpha)
	}

	actual fun getAlpha(): Float = layer?.getAlpha() ?: 0f

	actual fun setScissorRect(scissorRect: RectI?) {
		layer?.setScissorRect(scissorRect)
	}

	actual fun isReadyToRenderOffscreen(): LayerReadyState =
		layer?.isReadyToRenderOffscreen().asShared()

	actual fun enableAnimations(enabled: Boolean) {
		layer?.enableAnimations(enabled)
	}

	actual fun setErrorManager(errorManager: ErrorManager) {
		layer?.setErrorManager(errorManager.asMapscore())
	}

	actual fun forceReload() {
		layer?.forceReload()
	}

	actual fun setPrimaryRenderTarget(target: RenderTargetInterface?) {
		layer?.setPrimaryRenderTarget(target?.asMapscore())
	}
}

internal fun MapscoreLayerReadyState?.asShared(): LayerReadyState = when (this) {
	MapscoreLayerReadyState.READY -> LayerReadyState.READY
	MapscoreLayerReadyState.NOT_READY -> LayerReadyState.NOT_READY
	MapscoreLayerReadyState.ERROR -> LayerReadyState.ERROR
	MapscoreLayerReadyState.TIMEOUT_ERROR -> LayerReadyState.TIMEOUT_ERROR
	null -> LayerReadyState.NOT_READY
}
