package io.openmobilemaps.mapscore.kmp

import android.graphics.Bitmap
import io.openmobilemaps.mapscore.map.util.MapRenderHelper
import io.openmobilemaps.mapscore.map.util.MapViewRenderState
import io.openmobilemaps.mapscore.shared.graphics.common.Color
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.CoroutineScope

public data class AndroidOffscreenMapRenderBitmap(
	public val bitmap: Bitmap,
) : OffscreenMapRenderBitmap

private class AndroidOffscreenMapRenderHelper(
	private val mapConfig: KMMapConfig,
	private val coroutineScope: CoroutineScope,
) : OffscreenMapRenderHelper {
	override suspend fun renderMap(
		bounds: KMRectCoord,
		boundsPaddingPc: Float,
		renderSizePx: KMVec2I,
		renderDensity: Float,
		onSetupMap: (KMMapInterface) -> Unit,
	): OffscreenMapRenderResult {
		val deferredResult = CompletableDeferred<Bitmap>()

		MapRenderHelper.renderMap(
			coroutineScope = coroutineScope,
			mapConfig = mapConfig.asPlatform(),
			onSetupMap = { mapView ->
				mapView.setBackgroundColor(Color(1.0f, 1.0f, 1.0f, 1.0f))
				onSetupMap(mapView.requireMapInterface().asKmp())
			},
			onStateUpdate = { state ->
				when (state) {
					MapViewRenderState.Error ->
						deferredResult.completeExceptionally(
							OffscreenMapRenderException(OffscreenMapRenderExceptionType.ERROR),
						)

					is MapViewRenderState.Finished -> deferredResult.complete(state.bitmap)
					MapViewRenderState.Loading -> Unit
					MapViewRenderState.Timeout ->
						deferredResult.completeExceptionally(
							OffscreenMapRenderException(OffscreenMapRenderExceptionType.TIMEOUT),
						)
				}
			},
			renderBounds = bounds.asPlatform(),
			renderSizePx = renderSizePx.asPlatform(),
			renderDensity = renderDensity,
			renderTimeoutSeconds = 20f,
			destroyAfterRenderAction = true,
			renderBoundsPaddingPc = boundsPaddingPc,
		)

		return try {
			OffscreenMapRenderResult.Success(AndroidOffscreenMapRenderBitmap(deferredResult.await()))
		} catch (exception: Exception) {
			OffscreenMapRenderResult.Error(exception)
		}
	}
}

public actual fun createOffscreenMapRenderHelper(
	mapConfig: KMMapConfig,
	coroutineScope: CoroutineScope,
): OffscreenMapRenderHelper = AndroidOffscreenMapRenderHelper(
	mapConfig = mapConfig,
	coroutineScope = coroutineScope,
)
