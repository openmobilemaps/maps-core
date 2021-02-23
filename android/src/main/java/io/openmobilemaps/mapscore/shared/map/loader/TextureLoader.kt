/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

package io.openmobilemaps.mapscore.shared.map.loader

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Canvas
import android.graphics.Color
import io.openmobilemaps.mapscore.shared.graphics.BitmapTextureHolder
import okhttp3.*
import java.io.File
import java.util.*
import java.util.concurrent.TimeUnit

class TextureLoader(private val context: Context, private val cacheDirectory: File, private val cacheSize: Long) :
	TextureLoaderInterface() {

	private val errorBitmap = BitmapTextureHolder(Bitmap.createBitmap(2, 2, Bitmap.Config.ARGB_8888).apply {
		Canvas(this).apply { drawColor(Color.RED) }
	})

	private val okHttpClient = OkHttpClient.Builder()
		.connectionPool(ConnectionPool(8, 5000L, TimeUnit.MILLISECONDS))
		.cache(Cache(cacheDirectory, cacheSize))
		.dispatcher(Dispatcher().apply { maxRequestsPerHost = 8 })
		.readTimeout(20, TimeUnit.SECONDS)
		.build()

	override fun loadTexture(url: String): TextureLoaderResult {
		val request = Request.Builder()
			.url(url)
			.build()

		try {
			return okHttpClient.newCall(request).execute().use { response ->
				val bytes: ByteArray? = response.body?.bytes()
				if (response.isSuccessful && bytes != null) {
					val bitmap = BitmapFactory.decodeByteArray(bytes, 0, bytes.size);
					return@use TextureLoaderResult(BitmapTextureHolder(bitmap), LoaderStatus.OK)
				} else if (response.code == 404) {
					return@use TextureLoaderResult(null, LoaderStatus.ERROR_404)
				} else {
					return@use TextureLoaderResult(null, LoaderStatus.ERROR_OTHER)
				}
			}
		} catch (e: Exception) {
			return TextureLoaderResult(null, LoaderStatus.ERROR_NETWORK)
		}
	}
}