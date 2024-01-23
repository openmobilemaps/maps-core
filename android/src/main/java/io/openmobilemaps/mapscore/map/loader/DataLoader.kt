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
import java.util.concurrent.TimeUnit

open class DataLoader(
	context: Context,
	private var cacheDirectory: File,
	private var cacheSize: Long,
	private var referrer: String,
	private var userAgent: String = RequestUtils.getDefaultUserAgent(context)
) : LoaderInterface() {

	companion object {
		private const val HEADER_NAME_ETAG = "etag"
	}

	protected var okHttpClient = initializeClient()

	protected open fun createClient(): OkHttpClient = OkHttpClient.Builder()
		.addInterceptor(UserAgentInterceptor(userAgent))
		.addInterceptor(RefererInterceptor(referrer))
		.connectionPool(ConnectionPool(8, 5000L, TimeUnit.MILLISECONDS))
		.cache(Cache(cacheDirectory, cacheSize))
		.dispatcher(Dispatcher().apply { maxRequestsPerHost = 8 })
		.readTimeout(20, TimeUnit.SECONDS)
		.build()

	fun adjustClientSettings(
		cacheDirectory: File? = null,
		cacheSize: Long? = null,
		referrer: String? = null,
		userAgent: String? = null
	) {
		cacheDirectory?.let { this.cacheDirectory = cacheDirectory }
		cacheSize?.let { this.cacheSize = cacheSize }
		referrer?.let { this.referrer = it }
		userAgent?.let { this.userAgent = it }
		okHttpClient = createClient()
	}

	override fun loadTexture(url: String, etag: String?): TextureLoaderResult {
		val request = Request.Builder()
			.url(url)
			.build()

		try {
			return okHttpClient.newCall(request).execute().use { response ->
				val bytes: ByteArray? = response.body?.bytes()
				if (response.isSuccessful && bytes != null) {
					val bitmap = BitmapFactory.decodeByteArray(bytes, 0, bytes.size)
					return@use TextureLoaderResult(
						BitmapTextureHolder(bitmap),
						response.header(HEADER_NAME_ETAG, null),
						LoaderStatus.OK,
						null
					)
				} else if (response.code == 404) {
					return@use TextureLoaderResult(null, null, LoaderStatus.ERROR_404, response.code.toString())
				} else {
					return@use TextureLoaderResult(null, null, LoaderStatus.ERROR_OTHER, response.code.toString())
				}
			}
		} catch (e: Exception) {
			return TextureLoaderResult(null, null, LoaderStatus.ERROR_NETWORK, null)
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
					return@use DataLoaderResult(DataHolder(bytes), response.header(HEADER_NAME_ETAG, null), LoaderStatus.OK, null)
				} else if (response.code == 404) {
					return@use DataLoaderResult(null, null, LoaderStatus.ERROR_404, response.code.toString())
				} else {
					return@use DataLoaderResult(null, null, LoaderStatus.ERROR_OTHER, response.code.toString())
				}
			}
		} catch (e: Exception) {
			return DataLoaderResult(null, null, LoaderStatus.ERROR_NETWORK, null)
		}
	}

	private fun initializeClient() = createClient()
}
