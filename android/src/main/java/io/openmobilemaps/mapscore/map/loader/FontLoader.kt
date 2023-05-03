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

		val imageSize = fontJson.common.scaleW.toDouble()
		val size = fontJson.info.size.toDouble()

        val fontWrapper = FontWrapper(
            fontJson.info.name,
            fontJson.common.lineHeight.toDouble(),
            fontJson.common.base.toDouble(),
            Vec2D(imageSize, imageSize),
            size * dpFactor
        )

        val glyphs = fontJson.chars.map { glyphEntry ->
			var s0 = glyphEntry.x.toDouble()
			var s1 = s0 + glyphEntry.width.toDouble()
			var t0 = glyphEntry.y.toDouble()
			var t1 = t0 + glyphEntry.height.toDouble()

			s0 = s0 / imageSize
			s1 = s1 / imageSize
			t0 = t0 / imageSize
			t1 = t1 / imageSize

			val bearing = Vec2D(glyphEntry.xoffset.toDouble() / size, -glyphEntry.yoffset.toDouble() / size)
			val advance = Vec2D(glyphEntry.xadvance.toDouble() / size, 0.0)
			val bbox = Vec2D(glyphEntry.width.toDouble() / size, glyphEntry.height.toDouble() / size)

            FontGlyph(
                glyphEntry.char,
                advance,
                bbox,
                bearing,
                Quad2dD(
                    Vec2D(s0, 1.0 - t1),
                    Vec2D(s1, 1.0 - t1),
                    Vec2D(s1, 1.0 - t0),
                    Vec2D(s0, 1.0 - t0)
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
        val chars: List<FontGlyphJsonData>;
		val pages: List<String>; // unused
		val info: FontInfoData;
		val common: FontCommonData;
    )

    @JsonClass(generateAdapter = true)
    data class FontGlyphJsonData(
        val id: Int,
        val index: Int,
        val char: String,
        val width: Int,
        val height: Int,
        val xoffset: Int,
        val yoffset: Int,
        val xadvance: Int,
        val chnl: Int, // unused
        val x: Int,
        val y: Int
    )

	@JsonClass(generateAdapter = true)
	data class FontInfoData(
		val face: String,
		val size: Int,
		val bold: Int, // unused
		val italic: Int, // unused
		val charset: List<String>, // unused
		val unicode: Int, // unused
		val stretchH: Int, // unused
		val aa: Int, // unused
		val padding: List<Int>,
		val spacing: List<Int>,
		val outline: Int
	)

	@JsonClass(generateAdapter = true)
	data class FontCommonData(
		val lineHeight: Int,
		val base: Int,
		val scaleW: Int,
		val scaleH: Int,
		val pages: Int, // unused
		val packed: Int, // unused
		val alphaChnl: Int, // unused
		val redChnl: Int, // unused
		val blueChnl: Int, // unused
		val greenChnl: Int, // unused
		val distanceField: DistanceFieldData, // unused
		val kernings: List<Int>
	)

	@JsonClass(generateAdapter = true)
	data class DistanceFieldData(
		val fieldType: String,
		val distanceRange: Int
	)
}
