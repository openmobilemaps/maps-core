/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

package io.openmobilemaps.mapscore.shared.graphics

import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.drawable.Drawable
import android.opengl.GLES20
import android.opengl.GLUtils
import io.openmobilemaps.mapscore.shared.graphics.objects.TextureHolderInterface

class BitmapTextureHolder(bitmap: Bitmap) : TextureHolderInterface() {
	val bitmap: Bitmap
	private var imageWidth = 0
	private var imageHeight = 0
	private var textureWidth = 0
	private var textureHeight = 0

	constructor(drawable: Drawable) : this(createBitmapFromDrawable(drawable)) {}
	constructor(drawable: Drawable, targetWidth: Int, targetHeight: Int) : this(
		createBitmapFromDrawable(drawable, targetWidth, targetHeight)
	) {
	}

	override fun getImageWidth(): Int {
		return imageWidth
	}

	override fun getImageHeight(): Int {
		return imageHeight
	}

	override fun getTextureWidth(): Int {
		return textureWidth
	}

	override fun getTextureHeight(): Int {
		return textureHeight
	}

	override fun attachToGraphics() {
		GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, bitmap, 0)
	}

	companion object {
		private fun createBitmapFromDrawable(drawable: Drawable): Bitmap {
			return createBitmapFromDrawable(drawable, drawable.intrinsicWidth, drawable.intrinsicHeight)
		}

		private fun createBitmapFromDrawable(drawable: Drawable, targetWidth: Int, targetHeight: Int): Bitmap {
			val bitmap = Bitmap.createBitmap(targetWidth, targetHeight, Bitmap.Config.ARGB_8888)
			val canvas = Canvas(bitmap)
			drawable.setBounds(0, 0, canvas.width, canvas.height)
			drawable.draw(canvas)
			return bitmap
		}
	}

	init {
		var bitmap = bitmap
		var width = 2
		while (width < bitmap.width) {
			width *= 2
		}
		var height = 2
		while (height < bitmap.height) {
			height *= 2
		}
		imageWidth = bitmap.width
		imageHeight = bitmap.height
		textureWidth = width
		textureHeight = height
		if (bitmap.width != width || bitmap.height != height) {
			val large = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
			val c = Canvas(large)
			c.drawBitmap(bitmap, 0f, 0f, null)

			//Draw the picture mirrored again to fake clamp mode
			c.save()
			c.scale(1f, -1f, 0f, bitmap.height.toFloat())
			c.drawBitmap(bitmap, 0f, 0f, null)
			c.restore()
			c.save()
			c.scale(-1f, 1f, bitmap.width.toFloat(), 0f)
			c.drawBitmap(bitmap, 0f, 0f, null)
			c.restore()
			c.scale(-1f, -1f, bitmap.width.toFloat(), bitmap.height.toFloat())
			c.drawBitmap(bitmap, 0f, 0f, null)
			bitmap.recycle()
			bitmap = large
		}
		this.bitmap = bitmap
	}
}