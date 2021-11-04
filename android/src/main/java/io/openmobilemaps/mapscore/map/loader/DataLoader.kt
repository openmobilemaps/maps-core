/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

package io.openmobilemaps.mapscore.map.loader

import android.content.Context
import android.graphics.BitmapFactory
import io.openmobilemaps.mapscore.graphics.BitmapTextureHolder
import io.openmobilemaps.mapscore.graphics.DataHolder
import io.openmobilemaps.mapscore.map.loader.networking.RefererInterceptor
import io.openmobilemaps.mapscore.map.loader.networking.RequestUtils
import io.openmobilemaps.mapscore.map.loader.networking.UserAgentInterceptor
import io.openmobilemaps.mapscore.shared.map.loader.*
import okhttp3.*
import java.io.File
import java.util.*
import java.util.concurrent.TimeUnit

class DataLoader(
	private val context: Context,
	private val cacheDirectory: File,
	private val cacheSize: Long,
	private val referer: String,
	private val userAgent: String? = null
) : LoaderInterface() {

	companion object {
		private const val HEADER_NAME_ETAG = "etag"
	}

	private val okHttpClient = OkHttpClient.Builder()
		.addInterceptor(UserAgentInterceptor(userAgent ?: RequestUtils.getDefaultUserAgent(context)))
		.addInterceptor(RefererInterceptor(referer))
		.connectionPool(ConnectionPool(8, 5000L, TimeUnit.MILLISECONDS))
		.cache(Cache(cacheDirectory, cacheSize))
		.dispatcher(Dispatcher().apply { maxRequestsPerHost = 8 })
		.readTimeout(20, TimeUnit.SECONDS)
		.build()

	override fun loadTexture(url: String, etag: String?): TextureLoaderResult {
		val request = Request.Builder()
			.url(url)
			.build()

		try {
			return okHttpClient.newCall(request).execute().use { response ->
				val bytes: ByteArray? = response.body?.bytes()
				if (response.isSuccessful && bytes != null) {
					val bitmap = BitmapFactory.decodeByteArray(bytes, 0, bytes.size);
					return@use TextureLoaderResult(BitmapTextureHolder(bitmap), response.header(HEADER_NAME_ETAG, null), LoaderStatus.OK)
				} else if (response.code == 404) {
					return@use TextureLoaderResult(null, null, LoaderStatus.ERROR_404)
				} else {
					return@use TextureLoaderResult(null, null, LoaderStatus.ERROR_OTHER)
				}
			}
		} catch (e: Exception) {
			return TextureLoaderResult(null, null, LoaderStatus.ERROR_NETWORK)
		}
	}

	override fun loadData(url: String, etag: String?): DataLoaderResult {
		val request = Request.Builder()
			.url(url)
			.build()

		try {
			return okHttpClient.newCall(request).execute().use { response ->
				val bytes: ByteArray? = response.body?.bytes()
				if (response.isSuccessful && bytes != null) {
					return@use DataLoaderResult(DataHolder(bytes), response.header(HEADER_NAME_ETAG, null), LoaderStatus.OK)
				} else if (response.code == 404) {
					return@use DataLoaderResult(null, null, LoaderStatus.ERROR_404)
				} else {
					return@use DataLoaderResult(null, null, LoaderStatus.ERROR_OTHER)
				}
			}
		} catch (e: Exception) {
			return DataLoaderResult(null, null, LoaderStatus.ERROR_NETWORK)
		}
	}
}
