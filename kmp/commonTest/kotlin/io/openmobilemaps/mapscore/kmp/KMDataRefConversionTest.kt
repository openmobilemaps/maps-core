package io.openmobilemaps.mapscore.kmp

import kotlin.test.Test
import kotlin.test.assertNotNull

class KMDataRefConversionTest {
    @Test
    fun nonEmptyByteArrayConvertsToDataRef() {
        val dataRef = byteArrayOf(0x01, 0x02, 0x03).toKMDataRef()
        assertNotNull(dataRef)
    }

    @Test
    fun emptyByteArrayConvertsToDataRef() {
        val dataRef = ByteArray(0).toKMDataRef()
        assertNotNull(dataRef)
    }
}
