package io.openmobilemaps.mapscore.graphics

sealed class CanvasEdgeFillMode {
	data object Mirorred : CanvasEdgeFillMode()
	data class Clamped(val borderWidthPx: Int) : CanvasEdgeFillMode()
	data object None : CanvasEdgeFillMode()
}