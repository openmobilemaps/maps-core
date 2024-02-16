package io.openmobilemaps.mapscore.map.view

enum class MapViewState {
    UNINITIALIZED,  // Initial state
    INITIALIZED,    // State after setup
    RESUMED,        // State when the map is fully resumed
    PAUSED,         // State when the map is paused
    DESTROYED       // State after the MapView has been destroyed
}