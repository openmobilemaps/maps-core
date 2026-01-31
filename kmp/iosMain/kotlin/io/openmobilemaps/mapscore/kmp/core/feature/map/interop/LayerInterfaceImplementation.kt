package io.openmobilemaps.mapscore.kmp.feature.map.interop

import MapCoreSharedModule.MCLayerInterfaceProtocol
import MapCoreSharedModule.MCLayerReadyState
import MapCoreSharedModule.MCLayerReadyStateERROR
import MapCoreSharedModule.MCLayerReadyStateNOT_READY
import MapCoreSharedModule.MCLayerReadyStateREADY
import MapCoreSharedModule.MCLayerReadyStateTIMEOUT_ERROR

actual open class LayerInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
	private val layer = nativeHandle as? MCLayerInterfaceProtocol

	internal fun asMapCore(): MCLayerInterfaceProtocol? =
		nativeHandle as? MCLayerInterfaceProtocol

	actual fun setMaskingObject(maskingObject: MaskingObjectInterface?) {
		layer?.setMaskingObject(maskingObject?.asMapCore())
	}

	actual fun update() {
		layer?.update()
	}

	actual fun buildRenderPasses(): List<RenderPassInterface> =
		layer?.buildRenderPasses()?.map { RenderPassInterface(it) } ?: emptyList()

	actual fun buildComputePasses(): List<ComputePassInterface> =
		layer?.buildComputePasses()?.map { ComputePassInterface(it) } ?: emptyList()

	actual fun onAdded(mapInterface: MapInterface, layerIndex: Int) {
		layer?.onAdded(mapInterface.asMapCore(), layerIndex)
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
		layer?.setErrorManager(errorManager.asMapCore())
	}

	actual fun forceReload() {
		layer?.forceReload()
	}

	actual fun setPrimaryRenderTarget(target: RenderTargetInterface?) {
		layer?.setPrimaryRenderTarget(target?.asMapCore())
	}
}

private fun MCLayerReadyState?.asShared(): LayerReadyState = when (this) {
	MCLayerReadyStateREADY -> LayerReadyState.READY
	MCLayerReadyStateNOT_READY -> LayerReadyState.NOT_READY
	MCLayerReadyStateERROR -> LayerReadyState.ERROR
	MCLayerReadyStateTIMEOUT_ERROR -> LayerReadyState.TIMEOUT_ERROR
	null -> LayerReadyState.NOT_READY
	else -> LayerReadyState.NOT_READY
}
