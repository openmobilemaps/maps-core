package io.openmobilemaps.mapscore.map.util

import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Rect
import io.openmobilemaps.mapscore.graphics.BitmapTextureHolder
import io.openmobilemaps.mapscore.shared.graphics.RectanglePacker
import io.openmobilemaps.mapscore.shared.graphics.common.Vec2I
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.Tiled2dMapVectorAssetInfo
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.Tiled2dMapVectorLayerSymbolDelegateInterface
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.VectorLayerFeatureInfo

class CustomIconImageProvider(
    private val bitmapProvider: (featureInfo: VectorLayerFeatureInfo, layerIdentifier: String) -> Bitmap?
) : Tiled2dMapVectorLayerSymbolDelegateInterface() {

    companion object {
        const val MAX_SIZE_TEXTURE_PAGE_PX = 1024
    }

    override fun getCustomAssetsFor(
        featureInfos: ArrayList<VectorLayerFeatureInfo>,
        layerIdentifier: String
    ): ArrayList<Tiled2dMapVectorAssetInfo> {
        val bitmaps = featureInfos.associate { it.identifier to bitmapProvider(it, layerIdentifier) }
        val packerResult = RectanglePacker.pack(
            HashMap(bitmaps.mapValues { (identifier, bitmap) ->
                Vec2I(
                    (bitmap?.width ?: 1).toInt(),
                    (bitmap?.height ?: 1).toInt()
                )
            }), Vec2I(MAX_SIZE_TEXTURE_PAGE_PX, MAX_SIZE_TEXTURE_PAGE_PX)
        )

        return ArrayList(packerResult.map { page ->
            val pageAtlasBitmap = Bitmap.createBitmap(MAX_SIZE_TEXTURE_PAGE_PX, MAX_SIZE_TEXTURE_PAGE_PX, Bitmap.Config.ARGB_8888)
            val pageAtlasCanvas = Canvas(pageAtlasBitmap)
            page.uvs.forEach { (identifier, uv) ->
                bitmaps[identifier]?.let { bitmap ->
                    pageAtlasCanvas.drawBitmap(
                        bitmap,
                        Rect(0, 0, bitmap.width, bitmap.height),
                        Rect(uv.x, uv.y, uv.x + uv.width, uv.y + uv.height),
                        null
                    )
                }
            }

            Tiled2dMapVectorAssetInfo(page.uvs, BitmapTextureHolder(pageAtlasBitmap))
        })
    }

}