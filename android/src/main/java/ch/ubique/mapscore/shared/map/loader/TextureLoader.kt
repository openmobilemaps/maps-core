package ch.ubique.mapscore.shared.map.loader

import android.graphics.Bitmap
import android.graphics.Canvas
import ch.ubique.mapscore.shared.graphics.BitmapTextureHolder
import ch.ubique.mapscore.shared.graphics.objects.TextureHolderInterface
import java.util.*

class TextureLoader() : TextureLoaderInterface() {

	override fun loadTexture(url: String?): TextureHolderInterface {
		return BitmapTextureHolder(createBitmap())
	}

	fun createBitmap() : Bitmap {
		return Bitmap.createBitmap(256, 256, Bitmap.Config.ARGB_8888).apply {
			Canvas(this).apply {
				drawColor(Random().nextInt())
			}
		}
	}

}