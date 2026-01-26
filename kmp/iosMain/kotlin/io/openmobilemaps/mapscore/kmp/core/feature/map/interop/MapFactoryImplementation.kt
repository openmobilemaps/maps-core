package io.openmobilemaps.mapscore.kmp.feature.map.interop

import kotlin.experimental.ExperimentalObjCName
import kotlin.native.ObjCName

import MapCoreObjC.MCMapCoreObjCFactory
import MapCoreSharedModule.MCFontLoaderInterfaceProtocol
import MapCoreSharedModule.MCLoaderInterfaceProtocol
import MapCoreSharedModule.MCTiled2dMapRasterLayerInterface
import MapCoreSharedModule.MCTiled2dMapVectorLayerInterface
import platform.Foundation.NSBundle
import platform.Foundation.NSLog
import platform.Foundation.NSNumber

@OptIn(ExperimentalObjCName::class)
@ObjCName("MapFactory", exact = true)
actual abstract class MapFactory actual constructor(
	platformContext: Any?,
	coroutineScope: kotlinx.coroutines.CoroutineScope?,
	lifecycle: Any?,
) {
	protected val platformContext: Any? = platformContext
	protected val coroutineScope: kotlinx.coroutines.CoroutineScope? = coroutineScope
	protected val lifecycle: Any? = lifecycle

	actual abstract fun _createVectorLayer(
		layerName: String,
		dataProvider: MapDataProviderProtocol,
	): MapVectorLayer?
	actual abstract fun _createRasterLayer(config: MapTiled2dMapLayerConfig): MapRasterLayer?
	actual abstract fun _createGpsLayer(): MapGpsLayer?

	actual companion object {
		actual fun create(
			platformContext: Any?,
			coroutineScope: kotlinx.coroutines.CoroutineScope?,
			lifecycle: Any?,
		): MapFactory = MapFactoryImpl(platformContext, coroutineScope, lifecycle)
	}

	protected fun findMapBundle(): NSBundle? {
		return sharedResourcesBundle()
	}

	protected fun logMissing(resource: String) {
		NSLog("MapFactory: missing %s", resource)
	}

	protected fun sharedResourcesBundle(): NSBundle? {
		val path = NSBundle.mainBundle.pathForResource("SharedResources", ofType = "bundle") ?: return null
		return NSBundle(path)
	}

}

private class MapFactoryImpl(
	platformContext: Any?,
	coroutineScope: kotlinx.coroutines.CoroutineScope?,
	lifecycle: Any?,
) : MapFactory(platformContext, coroutineScope, lifecycle) {
	override fun _createVectorLayer(
		layerName: String,
		dataProvider: MapDataProviderProtocol,
	): MapVectorLayer? {
		val styleJson = dataProvider.getStyleJson() ?: run {
			logMissing("style json for $layerName")
			return null
		}
		val bundle = findMapBundle() ?: run {
			logMissing("bundle for MapFonts")
			return null
		}
		val fontLoader = MCMapCoreObjCFactory.createFontLoaderWithBundle(bundle) as? MCFontLoaderInterfaceProtocol
			?: run {
				logMissing("font loader")
				return null
			}
		val loader = MCMapCoreObjCFactory.createTextureLoader() as? MCLoaderInterfaceProtocol
			?: run {
				logMissing("texture loader")
				return null
			}
		val loaders = listOf(loader)
		val provider = MapDataProviderLocalDataProviderImplementation(dataProvider)
		val layer = MCTiled2dMapVectorLayerInterface.createExplicitly(
			layerName,
			styleJson = styleJson,
			localStyleJson = NSNumber(bool = true),
			loaders = loaders,
			fontLoader = fontLoader,
			localDataProvider = provider,
			customZoomInfo = null,
			symbolDelegate = null,
			sourceUrlParams = null,
		)
		return layer?.let { MapVectorLayerImpl(it) }
	}

	override fun _createRasterLayer(config: MapTiled2dMapLayerConfig): MapRasterLayer? {
		val loader = MCMapCoreObjCFactory.createTextureLoader() as? MCLoaderInterfaceProtocol
			?: run {
				logMissing("texture loader")
				return null
			}
		val loaders = listOf(loader)
		val layer = MCTiled2dMapRasterLayerInterface.create(
			MapTiled2dMapLayerConfigImplementation(config),
			loaders = loaders,
		)
		return layer?.let { MapRasterLayerImpl(it) }
	}

	override fun _createGpsLayer(): MapGpsLayer? {
		return MapGpsLayerImpl(null)
	}
}

private val fontNames = listOf(
	"Frutiger Neue Bold",
	"Frutiger Neue Condensed Bold",
	"Frutiger Neue Condensed Medium",
	"Frutiger Neue Condensed Regular",
	"Frutiger Neue Italic",
	"Frutiger Neue LT Condensed Bold",
	"Frutiger Neue Light",
	"Frutiger Neue Medium",
	"Frutiger Neue Regular",
	"FrutigerLTStd-Roman",
)
