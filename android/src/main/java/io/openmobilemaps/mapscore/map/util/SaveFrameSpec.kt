package io.openmobilemaps.mapscore.map.util

data class SaveFrameSpec(
	val targetWidthPx: Int? = null,
	val targetHeightPx: Int? = null,
	val cropLeftPc: Float = 0f,
	val cropTopPc: Float = 0f,
	val cropRightPc: Float = 0f,
	val cropBottomPc: Float = 0f,
	val outputFormat: OutputFormat = OutputFormat.BYTE_ARRAY,
	val pixelFormat: PixelFormat = PixelFormat.RGB_565
) {
	enum class OutputFormat {
		BITMAP,
		BYTE_ARRAY
	}

	enum class PixelFormat {
		RGB_565,
		ARGB_8888
	}
}