package io.openmobilemaps.mapscore.kmp.core.feature.map.interop

expect class IconLayerKmp {
	companion object {
		fun create() : IconLayerKmp
	}

	fun setIcons(icons: List<IconInfoInterfaceKmp>)

	fun getIcons(): List<IconInfoInterfaceKmp>

	fun remove(icon: IconInfoInterfaceKmp)

	fun removeList(icons: ArrayList<IconInfoInterfaceKmp>)

	fun removeIdentifier(identifier: String)

	fun removeIdentifierList(identifiers: ArrayList<String>)

	fun add(icon: IconInfoInterfaceKmp)

	fun addList(icons: ArrayList<IconInfoInterfaceKmp>)

	fun clear()

	fun setCallbackHandler(handler: IconLayerCallbackInterfaceKmp)

	fun asLayerInterface(): LayerInterfaceKmp

	fun invalidate()

	fun setLayerClickable(isLayerClickable: Boolean)

	fun setRenderPassIndex(index: Int)

	/** scale an icon, use repetitions for pulsating effect (repetions == -1 -> forever) */
	fun animateIconScale(identifier: String, from: Float, to: Float, duration: Float, repetitions: Int)
}

expect class IconInfoInterfaceKmp {
	fun getIdentifier(): String

	fun setCoordinate(coord: CoordKmp)

	// ...

}

expect class IconLayerCallbackInterfaceKmp {
	// ...
}

expect class LayerInterfaceKmp {
	// ...
}

expect class CoordKmp(systemIdentifier: Int, x: Double, y: Double, z: Double) {
	val systemIdentifier: Int
	val x: Double
	val y: Double
	val z: Double
}

expect object IconFactoryKmp {
	fun createIcon(): IconInfoInterfaceKmp
	fun createIconWithAnchor(): IconInfoInterfaceKmp
}

object TestMap {
	init {
		val iconLayer = IconLayerKmp.create()
		val icons = iconLayer.getIcons()
		val icons2 = iconLayer.getIcons()
		icons.forEach { it.setCoordinate(CoordKmp(4326, 0.0, 0.0, 0.0)) }

		iconLayer.add(IconFactoryKmp.createIcon())
	}
}