package io.openmobilemaps.mapscore.kmp

expect object KMMapInterfaceBridge {
    fun wrap(nativeHandle: Any): KMMapInterface
}
