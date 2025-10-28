package io.openmobilemaps.mapscore.extensions

import io.openmobilemaps.mapscore.shared.map.LayerReadyState

fun LayerReadyState?.isTerminal() = when (this) {
	LayerReadyState.READY,
	LayerReadyState.ERROR,
	LayerReadyState.TIMEOUT_ERROR,
		-> true
	LayerReadyState.NOT_READY,
	null,
		-> false
}