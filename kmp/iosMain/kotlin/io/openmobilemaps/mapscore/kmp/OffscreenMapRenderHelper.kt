@file:OptIn(kotlinx.cinterop.ExperimentalForeignApi::class)

package io.openmobilemaps.mapscore.kmp

import swiftPMImport.io.openmobilemaps.mapscore.kmp.MCMapView
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlinx.coroutines.withContext
import platform.CoreGraphics.CGSizeMake
import platform.UIKit.UIColor
import platform.UIKit.UIImage
import swiftPMImport.io.openmobilemaps.mapscore.kmp.renderToImageWithSize
import kotlin.coroutines.resume

private const val RENDER_TIMEOUT_SECONDS = 20f
private const val SECOND_RENDER_DELAY_MS = 350L

public data class IosOffscreenMapRenderBitmap(
	public val image: UIImage,
) : OffscreenMapRenderBitmap

private class IosOffscreenMapRenderHelper(
	private val mapConfig: KMMapConfig,
	private val coroutineScope: CoroutineScope,
) : OffscreenMapRenderHelper {
	override suspend fun renderMap(
		bounds: KMRectCoord,
		boundsPaddingPc: Float,
		renderSizePx: KMVec2I,
		renderDensity: Float,
		onSetupMap: (KMMapInterface) -> Unit,
	): OffscreenMapRenderResult = withContext(Dispatchers.Main) {
		val is3D = mapConfig.mapCoordinateSystem.identifier == KMCoordinateSystemIdentifiers.UnitSphere()

		// Honor the caller's `mapConfig` (coordinate system) instead of falling back to the
		// no-arg `MCMapView()` initializer, which hard-codes EPSG:3857. Also derive the view's
		// 3D mode from the coordinate system so unit-sphere renders use the globe pipeline.
		val mapView = MCMapView(
			mapConfig = mapConfig.asPlatform(),
			pixelsPerInch = null,
			is3D = is3D,
		)

		mapView.backgroundColor = UIColor.whiteColor
		onSetupMap(mapView.mapInterface.asKmp())

		suspend fun renderOnce(): OffscreenMapRenderResult = suspendCancellableCoroutine { continuation ->
			mapView.renderToImageWithSize(
				size = CGSizeMake(renderSizePx.x.toDouble(), renderSizePx.y.toDouble()),
				timeout = RENDER_TIMEOUT_SECONDS,
				bounds = bounds.asPlatform(),
				boundsPaddingPc = boundsPaddingPc,
				callbackQueue = null,
				callback = callback@ { image, state ->
					if (continuation.isCompleted) {
						return@callback
					}

					val result = when (KMLayerReadyState.fromPlatform(state)) {
						KMLayerReadyState.READY ->
							if (image != null) {
								OffscreenMapRenderResult.Success(IosOffscreenMapRenderBitmap(image))
							} else {
								OffscreenMapRenderResult.Error(
									OffscreenMapRenderException(OffscreenMapRenderExceptionType.ERROR),
								)
							}

						KMLayerReadyState.TIMEOUT_ERROR ->
							OffscreenMapRenderResult.Error(
								OffscreenMapRenderException(OffscreenMapRenderExceptionType.TIMEOUT),
							)

						KMLayerReadyState.ERROR,
						KMLayerReadyState.NOT_READY ->
							OffscreenMapRenderResult.Error(
								OffscreenMapRenderException(OffscreenMapRenderExceptionType.ERROR),
							)
					}
					continuation.resume(result)
				}
			)
		}

		val firstRenderResult = renderOnce()
		if (firstRenderResult is OffscreenMapRenderResult.Success) {
			delay(SECOND_RENDER_DELAY_MS)
			val secondRenderResult = renderOnce()
			if (secondRenderResult is OffscreenMapRenderResult.Success) {
				secondRenderResult
			} else {
				firstRenderResult
			}
		} else {
			firstRenderResult
		}
	}
}

public actual fun createOffscreenMapRenderHelper(
	mapConfig: KMMapConfig,
	coroutineScope: CoroutineScope,
): OffscreenMapRenderHelper = IosOffscreenMapRenderHelper(
	mapConfig = mapConfig,
	coroutineScope = coroutineScope,
)
