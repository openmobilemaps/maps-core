package ch.ubique.mapscore.shared.graphics;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.opengl.GLES20;
import android.opengl.GLUtils;

import ch.ubique.mapscore.shared.graphics.objects.TextureHolderInterface;

public class BitmapTextureHolder extends TextureHolderInterface {

	private Bitmap b;
	private int imageWidth;
	private int imageHeight;
	private int textureWidth;
	private int textureHeight;

	public BitmapTextureHolder(Drawable drawable) {
		this(createBitmapFromDrawable(drawable));
	}

	public BitmapTextureHolder(Drawable drawable, int targetWidth, int targetHeight) {
		this(createBitmapFromDrawable(drawable, targetWidth, targetHeight));
	}

	public BitmapTextureHolder(Bitmap bitmap) {
		if (bitmap != null) {
			int width = 2;
			while (width < bitmap.getWidth()) {
				width *= 2;
			}
			int height = 2;
			while (height < bitmap.getHeight()) {
				height *= 2;
			}

			imageWidth = bitmap.getWidth();
			imageHeight = bitmap.getHeight();
			textureWidth = width;
			textureHeight = height;

			if (bitmap.getWidth() != width || bitmap.getHeight() != height) {
				Bitmap large = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
				Canvas c = new Canvas(large);
				c.drawBitmap(bitmap, 0, 0, null);

				//Draw the picture mirrored again to fake clamp mode
				c.save();
				c.scale(1f, -1f, 0, bitmap.getHeight());
				c.drawBitmap(bitmap, 0, 0, null);
				c.restore();
				c.save();
				c.scale(-1f, 1f, bitmap.getWidth(), 0);
				c.drawBitmap(bitmap, 0, 0, null);
				c.restore();
				c.scale(-1f, -1f, bitmap.getWidth(), bitmap.getHeight());
				c.drawBitmap(bitmap, 0, 0, null);

				bitmap.recycle();
				bitmap = large;
			}
		}

		b = bitmap;
	}

	@Override
	public int getImageWidth() {
		return imageWidth;
	}

	@Override
	public int getImageHeight() {
		return imageHeight;
	}

	@Override
	public int getTextureWidth() {
		return textureWidth;
	}

	@Override
	public int getTextureHeight() {
		return textureHeight;
	}

	@Override
	public void attachToGraphics() {
		if (b != null) {
			GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, b, 0);
			b.recycle();
		}
	}

	public Bitmap getBitmap() {
		return b;
	}

	private static Bitmap createBitmapFromDrawable(Drawable drawable) {
		Bitmap bitmap = Bitmap.createBitmap(drawable.getIntrinsicWidth(),
				drawable.getIntrinsicHeight(), Bitmap.Config.ARGB_8888);
		return createBitmapFromDrawable(drawable, drawable.getIntrinsicWidth(),  drawable.getIntrinsicHeight());
	}

	private static Bitmap createBitmapFromDrawable(Drawable drawable, int targetWidth, int targetHeight) {
		Bitmap bitmap = Bitmap.createBitmap(targetWidth, targetHeight, Bitmap.Config.ARGB_8888);
		Canvas canvas = new Canvas(bitmap);
		drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
		drawable.draw(canvas);
		return bitmap;
	}

}

