package io.openmobilemaps.mapscore.kmp

actual object KMMapInterfaceBridge {
    actual fun wrap(nativeHandle: Any): KMMapInterface = KMMapInterface(nativeHandle)
}
