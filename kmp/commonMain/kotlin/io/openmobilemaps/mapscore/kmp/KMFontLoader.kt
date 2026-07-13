package io.openmobilemaps.mapscore.kmp

import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import kotlinx.serialization.json.Json
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.double
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

private val fontJson = Json {
	ignoreUnknownKeys = true
}

// A KMP Font Loader which loads the given font files from caller-provided bytes.
public class KMFontLoader(
	private val fontNames: List<String>,
	private val kmResources: KMResources,
	private val resourceRoot: String = "files/map/fonts",
	private val preloadScope: CoroutineScope = CoroutineScope(SupervisorJob() + Dispatchers.Default),
) : KMFontLoaderInterface {
	private companion object {
		private const val JSON_POSTFIX = ".json"
		private const val TEXTURE_POSTFIX = ".png"
	}

	private class LoaderResult(
		val imageData: ByteArray?,
		val fontData: KMFontData?,
		val status: KMLoaderStatus) {}

	private val preloadBarrier = CompletableDeferred<Map<String, LoaderResult>>()

	init {
		preloadScope.launch {
			val loadedFonts = runCatching { preloadFonts() }
				.onFailure { error ->
					println("KMFontLoader: preload failed: ${error.message}")
				}
				.getOrDefault(emptyMap())
			preloadBarrier.complete(loadedFonts)
		}
	}

	override fun loadFont(font: KMFont): KMFontLoaderResult {
		val loadedFonts = runBlocking {
			preloadBarrier.await()
		}

        return loadedFonts[font.name].let {
            val texture = it?.imageData?.let { imageBytes ->
				runCatching {
					decodeTextureHolder(imageBytes)
				}.getOrNull()
            } ?: return KMFontLoaderResult(
                imageData = null,
                fontData = null,
                status = KMLoaderStatus.ERROR_OTHER,
            );
            KMFontLoaderResult(
                imageData = texture,
                fontData = it.fontData,
                status = it.status
            )
        }
	}

	private suspend fun preloadFonts(): Map<String, LoaderResult> {
		val loadedFonts = mutableMapOf<String, LoaderResult>()
		for (fontName in fontNames) {
			val result = runCatching {
				val fontJsonText = kmResources.readBytes(resourcePath(fontName + JSON_POSTFIX)).getOrNull()?.decodeToString() ?: continue
				val textureBytes = kmResources.readBytes(resourcePath(fontName + TEXTURE_POSTFIX)).getOrNull() ?: continue

				val fontData = parseFontData(fontJsonText = fontJsonText, fallbackName = fontName)
				LoaderResult(
						imageData = textureBytes,
						fontData = fontData,
						status = KMLoaderStatus.OK,
					)
			}.getOrElse {
				LoaderResult(
					imageData = null,
					fontData = null,
					status = KMLoaderStatus.ERROR_OTHER,
				)
			}

			loadedFonts[fontName] = result
		}

		return loadedFonts
	}

	private fun resourcePath(fileName: String): String = "$resourceRoot/$fileName"
}

internal fun parseFontData(fontJsonText: String, fallbackName: String): KMFontData {
	val parsed = fontJson.parseToJsonElement(fontJsonText).jsonObject
	val info = parsed.requiredObject("info")
	val common = parsed.requiredObject("common")
	val distanceField = parsed.requiredObject("distanceField")
	val chars = parsed.requiredArray("chars")

	val fontSize = info.requiredDouble("size")
	val imageSize = common.requiredDouble("scaleW")
	val fontName = info.requiredString("face").ifBlank { fallbackName }

	val fontWrapper = KMFontWrapper(
		name = fontName,
		lineHeight = common.requiredDouble("lineHeight") / fontSize,
		base = common.requiredDouble("base") / fontSize,
		bitmapSize = KMVec2D(imageSize, imageSize),
		size = fontSize,
		distanceRange = distanceField.requiredDouble("distanceRange"),
	)

	val glyphs = chars.map { glyphElement ->
		val glyph = glyphElement.jsonObject
		var s0 = glyph.requiredDouble("x")
		var s1 = s0 + glyph.requiredDouble("width")
		var t0 = glyph.requiredDouble("y")
		var t1 = t0 + glyph.requiredDouble("height")

		s0 /= imageSize
		s1 /= imageSize
		t0 /= imageSize
		t1 /= imageSize

		val bearing = KMVec2D(glyph.requiredDouble("xoffset") / fontSize, -glyph.requiredDouble("yoffset") / fontSize)
		val advance = KMVec2D(glyph.requiredDouble("xadvance") / fontSize, 0.0)
		val boundingBox = KMVec2D(glyph.requiredDouble("width") / fontSize, glyph.requiredDouble("height") / fontSize)

		KMFontGlyph(
			charCode = glyph.requiredString("char"),
			advance = advance,
			boundingBoxSize = boundingBox,
			bearing = bearing,
			uv = KMQuad2dD(
				topLeft = KMVec2D(s0, t1),
				topRight = KMVec2D(s1, t1),
				bottomRight = KMVec2D(s1, t0),
				bottomLeft = KMVec2D(s0, t0),
			),
		)
	}

	return KMFontData(
		info = fontWrapper,
		glyphs = ArrayList(glyphs),
	)
}

private fun JsonObject.requiredObject(key: String): JsonObject =
	this[key]?.jsonObject ?: error("Missing object '$key'")

private fun JsonObject.requiredArray(key: String) =
	this[key]?.jsonArray ?: error("Missing array '$key'")

private fun JsonObject.requiredString(key: String): String =
	this[key]?.jsonPrimitive?.content ?: error("Missing string '$key'")

private fun JsonObject.requiredDouble(key: String): Double =
	this[key]?.jsonPrimitive?.double ?: error("Missing number '$key'")
