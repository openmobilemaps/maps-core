package io.openmobilemaps.mapscore.kmp.feature.map.interop

interface MapCallbackInterface {
    fun invalidate()
    fun onMapResumed()
}

interface MapReadyCallbackInterface {
    fun stateDidUpdate(state: LayerReadyState)
}

enum class LayerReadyState {
    READY,
    NOT_READY,
    ERROR,
    TIMEOUT_ERROR,
}
