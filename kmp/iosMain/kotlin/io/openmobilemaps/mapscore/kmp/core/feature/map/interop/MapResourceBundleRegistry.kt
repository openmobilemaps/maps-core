package io.openmobilemaps.mapscore.kmp.feature.map.interop

import platform.Foundation.NSBundle

object MapResourceBundleRegistry {
	var bundleProvider: (() -> NSBundle?)? = null
}
