package io.openmobilemaps.mapscore.map.util

import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Rect
import io.openmobilemaps.mapscore.graphics.BitmapTextureHolder
import io.openmobilemaps.mapscore.shared.graphics.RectanglePacker
import io.openmobilemaps.mapscore.shared.graphics.TextureAtlas
import io.openmobilemaps.mapscore.shared.graphics.common.Vec2I

object TextureAtlasProvider {
    fun createTextureAtlas(
        bitmaps: Map<String, Bitmap>,
        maxAtlasSize: Int = 1024,
		spacing: Int = 1
    ): ArrayList<TextureAtlas> {
        val packerResult = RectanglePacker.pack(
            HashMap(bitmaps.mapValues { (identifier, bitmap) ->
                Vec2I(bitmap.width, bitmap.height)
            }), Vec2I(maxAtlasSize, maxAtlasSize),
			spacing
        )

        return ArrayList(packerResult.map { page ->
            val atlasWidth = page.uvs.maxOf { it.value.x + it.value.width }
            val atlasHeight = page.uvs.maxOf { it.value.y + it.value.height }
            val pageAtlasBitmap = Bitmap.createBitmap(atlasWidth, atlasHeight, Bitmap.Config.ARGB_8888)
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

            TextureAtlas(page.uvs, BitmapTextureHolder(pageAtlasBitmap))
        })
    }

}
