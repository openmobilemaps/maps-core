/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

package io.openmobilemaps.mapscore.graphics

import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.drawable.Drawable
import android.opengl.GLES20
import android.opengl.GLES30
import android.opengl.GLUtils
import android.util.Log
import io.openmobilemaps.mapscore.shared.graphics.objects.TextureHolderInterface
import java.nio.IntBuffer
import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.locks.ReentrantLock

class BitmapTextureHolder(bitmap: Bitmap, val minFilter:Int = GLES20.GL_LINEAR, val magFilter:Int = GLES20.GL_LINEAR) : TextureHolderInterface() {
	val bitmap: Bitmap
	private var imageWidth = 0
	private var imageHeight = 0
	private var textureWidth = 0
	private var textureHeight = 0

	private val dataMutex = ReentrantLock()
	private var usageCounter = 0
	private var texturePointer: IntArray = intArrayOf(0)

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

	override fun attachToGraphics(): Int {
		dataMutex.lock()
		try {
			if (usageCounter++ == 0) {
				val texture = IntArray(1)
				GLES20.glGenTextures(1, texture, 0)
				GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture[0])
				GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, minFilter)
				GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, magFilter)
				GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE)
				GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE)
				GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, bitmap, 0)
				texturePointer = texture
			}
		} finally {
			dataMutex.unlock()
		}
		return texturePointer[0]
	}

	override fun clearFromGraphics() {
		dataMutex.lock()
		try {
			if (--usageCounter == 0) {
				GLES20.glDeleteTextures(1, IntBuffer.wrap(texturePointer))
			}
		} finally {
			dataMutex.unlock()
		}
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