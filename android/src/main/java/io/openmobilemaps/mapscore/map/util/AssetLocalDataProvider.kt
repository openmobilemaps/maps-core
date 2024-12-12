package io.openmobilemaps.mapscore.map.util

import android.content.res.AssetManager
import android.graphics.BitmapFactory
import android.util.Log
import com.snapchat.djinni.Future
import com.snapchat.djinni.Promise
import io.openmobilemaps.mapscore.graphics.BitmapTextureHolder
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.Tiled2dMapVectorLayerLocalDataProviderInterface
import io.openmobilemaps.mapscore.shared.map.loader.DataLoaderResult
import io.openmobilemaps.mapscore.shared.map.loader.LoaderStatus
import io.openmobilemaps.mapscore.shared.map.loader.TextureLoaderResult
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import java.io.FileNotFoundException
import java.io.IOException
import java.nio.ByteBuffer

open class AssetLocalDataProvider(
    private val assets: AssetManager,
    private val coroutineScope: CoroutineScope,
    private val folderAssetsBase: String,
    private val fileNameStyleJson: String = "style.json",
    private val folderSprite: String = "generated-sprites",
    private val fileNameSprite: String = "sprite",
    private val loadDataAsync: ((url: String, etag: String?) -> Future<DataLoaderResult>)? = null
) : Tiled2dMapVectorLayerLocalDataProviderInterface() {

    protected open fun scaleSuffix(scale: Int): String = when (scale) {
        in 2..3 -> "@${scale}x"
        else -> ""
    }

    protected open fun modifyLoadedStyleJson(styleJson: String): String = styleJson

    override fun getStyleJson(): String? {
        val assetPath = "${folderAssetsBase}/$fileNameStyleJson"
        return try {
            val assetStream = assets.open(assetPath)
            val styleJson = assetStream.bufferedReader(Charsets.UTF_8).readText()
            assetStream.close()
            modifyLoadedStyleJson(styleJson)
        } catch (e: IOException) {
            Log.e(AssetLocalDataProvider::class.java.canonicalName, e.localizedMessage, e)
            null
        } catch (e: FileNotFoundException) {
            Log.e(AssetLocalDataProvider::class.java.canonicalName, e.localizedMessage, e)
            null
        }
    }

    override fun loadSpriteAsync(scale: Int): Future<TextureLoaderResult> {
        val resultPromise = Promise<TextureLoaderResult>()

        coroutineScope.launch(Dispatchers.IO) {
            val assetPath = "${folderAssetsBase}/${folderSprite}/${fileNameSprite}${scaleSuffix(scale)}.png"
            val texture = try {
                val assetStream = assets.open(assetPath)
                val decodedTexture = BitmapFactory.decodeStream(assetStream)?.let { BitmapTextureHolder(it) }
                assetStream.close()
                decodedTexture
            } catch (e: IOException) {
                Log.e(AssetLocalDataProvider::class.java.canonicalName, e.localizedMessage, e)
                null
            } catch (e: FileNotFoundException) {
                Log.e(AssetLocalDataProvider::class.java.canonicalName, e.localizedMessage, e)
                null
            }
            val result = texture?.let { TextureLoaderResult(it, null, LoaderStatus.OK, null) }
                ?: TextureLoaderResult(null, null, LoaderStatus.ERROR_OTHER, null)
            resultPromise.setValue(result)
        }

        return resultPromise.future
    }

    override fun loadSpriteJsonAsync(scale: Int): Future<DataLoaderResult> {
        val resultPromise = Promise<DataLoaderResult>()

        coroutineScope.launch(Dispatchers.IO) {
            val assetPath = "${folderAssetsBase}/${folderSprite}/${fileNameSprite}${scaleSuffix(scale)}.json"
            val spriteJson = try {
                val assetStream = assets.open(assetPath)
                val bytes = assetStream.readBytes()
                val json = ByteBuffer.allocateDirect(bytes.size).put(bytes)
                assetStream.close()
                json
            } catch (e: IOException) {
                Log.e(AssetLocalDataProvider::class.java.canonicalName, e.localizedMessage, e)
                null
            } catch (e: FileNotFoundException) {
                Log.e(AssetLocalDataProvider::class.java.canonicalName, e.localizedMessage, e)
                null
            }
            val result = spriteJson?.let { DataLoaderResult(spriteJson, null, LoaderStatus.OK, null) }
                ?: DataLoaderResult(null, null, LoaderStatus.ERROR_OTHER, null)
            resultPromise.setValue(result)
        }

        return resultPromise.future
    }

    override fun loadGeojson(sourceName: String, url: String): Future<DataLoaderResult> {
        return loadDataAsync?.invoke(url, null) ?: Promise<DataLoaderResult>().apply {
            setValue(
                DataLoaderResult(
                    null,
                    null,
                    LoaderStatus.ERROR_404,
                    "No geoJson provided in AssetLocalDataProvider!"
                )
            )
        }.future
    }

    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false

        other as AssetLocalDataProvider

        return (folderAssetsBase == other.folderAssetsBase &&
                fileNameStyleJson == other.fileNameStyleJson &&
                folderSprite == other.folderSprite &&
                fileNameSprite == other.fileNameSprite)
    }
}