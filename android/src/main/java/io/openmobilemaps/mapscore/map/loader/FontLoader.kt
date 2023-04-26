package io.openmobilemaps.mapscore.map.loader

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import com.squareup.moshi.JsonClass
import com.squareup.moshi.Moshi
import io.openmobilemaps.mapscore.graphics.BitmapTextureHolder
import io.openmobilemaps.mapscore.shared.graphics.common.Quad2dD
import io.openmobilemaps.mapscore.shared.graphics.common.Vec2D
import io.openmobilemaps.mapscore.shared.map.loader.*

open class FontLoader(context: Context, private val dpFactor: Float) : FontLoaderInterface() {

    private val moshi = Moshi.Builder().build();
    private val moshiFontAdapter = moshi.adapter(FontDataJson::class.java)

    private val fontMap: MutableMap<String, FontDataHolder> = mutableMapOf()

    fun addFont(fontDataJson: String, fontAtlas: Bitmap) {

        val fontJson =
            moshiFontAdapter.fromJson(fontDataJson) ?: throw IllegalArgumentException("Invalid json format for font data!")

        val fontWrapper = FontWrapper(
            fontJson.name,
            fontJson.ascender,
            fontJson.descender,
            fontJson.space_advance,
            Vec2D(fontJson.bitmap_width.toDouble(), fontJson.bitmap_height.toDouble()),
            fontJson.size.toDouble() * dpFactor
        )
        val glyphs = fontJson.glyph_data.map { glyphEntry ->
            val glyphJson = glyphEntry.value
            FontGlyph(
                glyphJson.charcode,
                Vec2D(glyphJson.advance_x, glyphJson.advance_y ?: 0.0),
                Vec2D(glyphJson.bbox_width, glyphJson.bbox_height),
                Vec2D(glyphJson.bearing_x, glyphJson.bearing_y),
                Quad2dD(
                    Vec2D(glyphJson.s0, 1.0 - glyphJson.t1),
                    Vec2D(glyphJson.s1, 1.0 - glyphJson.t1),
                    Vec2D(glyphJson.s1, 1.0 - glyphJson.t0),
                    Vec2D(glyphJson.s0, 1.0 - glyphJson.t0)
                )
            )
        }.toCollection(ArrayList())
        fontMap[fontJson.name] = FontDataHolder(BitmapTextureHolder(fontAtlas), FontData(fontWrapper, glyphs))
    }

    fun addFont(fontData: FontData, fontAtlas: BitmapTextureHolder) {
        fontMap[fontData.info.name] = FontDataHolder(fontAtlas, fontData)
    }

    override fun loadFont(font: Font): FontLoaderResult =
        fontMap[font.name]?.let { fontDataHolder ->
            FontLoaderResult(fontDataHolder.fontTexture, fontDataHolder.fontData, LoaderStatus.OK)
        } ?: FontLoaderResult(null, null, LoaderStatus.ERROR_OTHER)


    private data class FontDataHolder(val fontTexture: BitmapTextureHolder, val fontData: FontData)

    @JsonClass(generateAdapter = true)
    data class FontDataJson(
        val name: String,
        val bitmap_height: Int,
        val bitmap_width: Int,
        val size: Int,
        val height: Double,
        val ascender: Double,
        val descender: Double,
        val space_advance: Double,
        val max_advance: Double,
        val glyph_data: Map<String, FontGlyphJsonData>
    )

    @JsonClass(generateAdapter = true)
    data class FontGlyphJsonData(
        val advance_x: Double,
        val advance_y: Double?,
        val bbox_height: Double,
        val bbox_width: Double,
        val bearing_x: Double,
        val bearing_y: Double,
        val charcode: String,
        val s0: Double,
        val s1: Double,
        val t0: Double,
        val t1: Double
    )

}
