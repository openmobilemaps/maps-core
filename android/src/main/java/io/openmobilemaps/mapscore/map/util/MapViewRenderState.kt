package io.openmobilemaps.mapscore.map.util

import android.graphics.Bitmap

sealed class MapViewRenderState {
	object Loading : MapViewRenderState()
	data class Finished(val bitmap: Bitmap) : MapViewRenderState()
	object Error : MapViewRenderState()
	object Timeout : MapViewRenderState()
}


