package io.openmobilemaps.mapscore.map.layers

import android.content.Context
import io.openmobilemaps.mapscore.map.loader.DataLoader
import io.openmobilemaps.mapscore.map.loader.FontLoader
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.Tiled2dMapVectorLayerInterface
import io.openmobilemaps.mapscore.shared.map.loader.FontLoaderInterface
import io.openmobilemaps.mapscore.shared.map.loader.LoaderInterface
import java.util.UUID

class TiledVectorLayer(
	styleUrl: String,
	loaders: ArrayList<LoaderInterface>,
	fontLoader: FontLoaderInterface,
	name: String = UUID.randomUUID().toString()
) {

	constructor(
		context: Context,
		styleUrl: String,
		fontAssetFolder: String,
		loaders: ArrayList<LoaderInterface> = arrayListOf(DataLoader(context, context.cacheDir, 50 * 1024L * 1024L))
	) : this(styleUrl, loaders, FontLoader(context, fontAssetFolder))

	constructor(
		context: Context,
		styleUrl: String,
		loaders: ArrayList<LoaderInterface> = arrayListOf(DataLoader(context, context.cacheDir, 50 * 1024L * 1024L)),
		fontLoader: FontLoaderInterface = FontLoader(context)
	) : this(styleUrl, loaders, fontLoader)

	private val layer = Tiled2dMapVectorLayerInterface.createFromStyleJson(name, styleUrl, loaders, fontLoader)

	fun layerInterface() = layer.asLayerInterface()
	fun vectorLayerInterface() = layer

}