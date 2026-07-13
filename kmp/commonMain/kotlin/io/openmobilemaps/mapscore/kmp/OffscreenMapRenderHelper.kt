package io.openmobilemaps.mapscore.kmp

import kotlinx.coroutines.CoroutineScope

public sealed class OffscreenMapRenderResult {
	public class Error(public val exception: Exception) : OffscreenMapRenderResult()

	public class Success(public val bitmap: OffscreenMapRenderBitmap) : OffscreenMapRenderResult()
}

public interface OffscreenMapRenderBitmap

public class OffscreenMapRenderException(
	public val type: OffscreenMapRenderExceptionType,
) : Exception("Offscreen map render failed: $type")

public enum class OffscreenMapRenderExceptionType {
	TIMEOUT,
	ERROR,
}

public interface OffscreenMapRenderHelper {
	public suspend fun renderMap(
		bounds: KMRectCoord,
		boundsPaddingPc: Float = 0.1f,
		renderSizePx: KMVec2I,
		renderDensity: Float,
		onSetupMap: (KMMapInterface) -> Unit = {},
	): OffscreenMapRenderResult
}

public expect fun createOffscreenMapRenderHelper(
	mapConfig: KMMapConfig,
	coroutineScope: CoroutineScope,
): OffscreenMapRenderHelper
