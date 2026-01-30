package io.openmobilemaps.mapscore.kmp.core.feature.map.interop

import io.openmobilemaps.mapscore.shared.map.coordinates.Coord
import io.openmobilemaps.mapscore.shared.map.layers.icon.IconFactory
import io.openmobilemaps.mapscore.shared.map.layers.icon.IconInfoInterface
import io.openmobilemaps.mapscore.shared.map.layers.icon.IconLayerInterface

actual class IconLayerKmp(val iconLayerCpp: IconLayerInterface) {
	actual companion object {
		actual fun create(): IconLayerKmp {
			return IconLayerKmp(IconLayerInterface.create())
		}
	}

	actual fun setIcons(icons: List<IconInfoInterfaceKmp>) {
		iconLayerCpp.setIcons(icons.map { it.iconInfoInterfaceCpp }.toCollection(ArrayList()))
	}

	actual fun getIcons(): List<IconInfoInterfaceKmp> {
		return iconLayerCpp.getIcons().map { IconInfoInterfaceKmp(it) }
	}

	actual fun remove(icon: IconInfoInterfaceKmp) {
	}

	actual fun removeList(icons: ArrayList<IconInfoInterfaceKmp>) {
	}

	actual fun removeIdentifier(identifier: String) {
	}

	actual fun removeIdentifierList(identifiers: ArrayList<String>) {
	}

	actual fun add(icon: IconInfoInterfaceKmp) {
	}

	actual fun addList(icons: ArrayList<IconInfoInterfaceKmp>) {
	}

	actual fun clear() {
	}

	actual fun setCallbackHandler(handler: IconLayerCallbackInterfaceKmp) {
	}

	actual fun asLayerInterface(): LayerInterfaceKmp {
		TODO("Not yet implemented")
	}

	actual fun invalidate() {
	}

	actual fun setLayerClickable(isLayerClickable: Boolean) {
	}

	actual fun setRenderPassIndex(index: Int) {
	}

	actual fun animateIconScale(identifier: String, from: Float, to: Float, duration: Float, repetitions: Int) {
	}
}

actual class IconInfoInterfaceKmp(val iconInfoInterfaceCpp : IconInfoInterface) {
	actual fun getIdentifier(): String {
		return iconInfoInterfaceCpp.getIdentifier()
	}

	actual fun setCoordinate(coord: CoordKmp) {
		iconInfoInterfaceCpp.setCoordinate(coord)
	}
	// ...
}

actual class IconLayerCallbackInterfaceKmp
actual class LayerInterfaceKmp

actual typealias CoordKmp = Coord

actual object IconFactoryKmp {
	actual fun createIcon(): IconInfoInterfaceKmp {
		return IconInfoInterfaceKmp(IconFactory.createIcon())
	}

	actual fun createIconWithAnchor(): IconInfoInterfaceKmp {
		return IconInfoInterfaceKmp(IconFactory.createIconWithAnchor())
	}
}

