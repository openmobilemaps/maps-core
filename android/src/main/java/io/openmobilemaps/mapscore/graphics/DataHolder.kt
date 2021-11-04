package io.openmobilemaps.mapscore.graphics

import io.openmobilemaps.mapscore.shared.map.loader.DataHolderInterface

class DataHolder(private val data: ByteArray) : DataHolderInterface() {
	override fun getData(): ByteArray {
		return data
	}
}