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
import com.snapchat.djinni.Future
import com.snapchat.djinni.Promise
import io.openmobilemaps.mapscore.graphics.BitmapTextureHolder
import io.openmobilemaps.mapscore.map.loader.networking.RefererInterceptor
import io.openmobilemaps.mapscore.map.loader.networking.RequestUtils
import io.openmobilemaps.mapscore.map.loader.networking.UserAgentInterceptor
import io.openmobilemaps.mapscore.shared.map.loader.*
import okhttp3.*
import java.io.File
import java.io.IOException
import java.nio.ByteBuffer
import java.util.concurrent.ExecutionException
import java.util.concurrent.TimeUnit
import java.util.concurrent.TimeoutException

open class DataLoader(
	context: Context,
	private var cacheDirectory: File,
	private var cacheSize: Long,
	private var referrer: String,
	private var userAgent: String = RequestUtils.getDefaultUserAgent(context)
) : LoaderInterface() {

	companion object {
		private const val HEADER_NAME_ETAG = "etag"

        private const val TIMEOUT_DUR = 20L
        private val TIMEOUT_UNIT = TimeUnit.SECONDS
	}

	protected var okHttpClient = initializeClient()

	protected open fun createClient(): OkHttpClient = OkHttpClient.Builder()
		.addInterceptor(UserAgentInterceptor(userAgent))
		.addInterceptor(RefererInterceptor(referrer))
		.connectionPool(ConnectionPool(8, 5000L, TimeUnit.MILLISECONDS))
		.cache(Cache(cacheDirectory, cacheSize))
		.dispatcher(Dispatcher().apply { maxRequestsPerHost = 8 })
		.readTimeout(TIMEOUT_DUR, TIMEOUT_UNIT)
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
        val resFuture = loadTextureAsnyc(url, etag)
        return try {
            resFuture.get(TIMEOUT_DUR, TIMEOUT_UNIT)
        } catch (e: Exception) {
            val status = when (e) {
                is InterruptedException, is ExecutionException -> LoaderStatus.ERROR_OTHER
                is TimeoutException -> LoaderStatus.ERROR_TIMEOUT
                else -> throw e
            }
            TextureLoaderResult(null, null, status, null)
        }
	}

    override fun loadTextureAsnyc(url: String, etag: String?): Future<TextureLoaderResult> {
        val request = Request.Builder()
            .url(url)
            .tag(url)
            .build()

        val result = Promise<TextureLoaderResult>()

        okHttpClient.newCall(request).enqueue(object : Callback {
            override fun onResponse(call: Call, response: Response) {
                val bytes: ByteArray? = response.body?.bytes()
                if (response.isSuccessful && bytes != null) {
                    val bitmap = BitmapFactory.decodeByteArray(bytes, 0, bytes.size)
                    result.setValue(
                        TextureLoaderResult(
                            BitmapTextureHolder(bitmap),
                            response.header(HEADER_NAME_ETAG, null),
                            LoaderStatus.OK,
                            null
                        )
                    )
                } else if (response.code == 404) {
                    result.setValue(TextureLoaderResult(null, null, LoaderStatus.ERROR_404, response.code.toString()))
                } else {
                    result.setValue(TextureLoaderResult(null, null, LoaderStatus.ERROR_OTHER, response.code.toString()))
                }
            }

			override fun onFailure(call: Call, e: IOException) {
				result.setValue(TextureLoaderResult(null, null, LoaderStatus.ERROR_NETWORK, null))
			}
		})
		return result.future
	}

	override fun loadData(url: String, etag: String?): DataLoaderResult {
		val resFuture = loadDataAsync(url, etag)
		return try {
			resFuture.get(TIMEOUT_DUR, TIMEOUT_UNIT)
		} catch (e: Exception) {
			val status = when (e) {
				is InterruptedException, is ExecutionException -> LoaderStatus.ERROR_OTHER
				is TimeoutException -> LoaderStatus.ERROR_TIMEOUT
				else -> throw e
			}
			DataLoaderResult(null, null, status, null)
		}
	}

	override fun loadDataAsync(url: String, etag: String?): Future<DataLoaderResult> {
		val request = Request.Builder()
			.url(url)
			.tag(url)
			.build()
		val result = Promise<DataLoaderResult>()
		okHttpClient.newCall(request).enqueue(object : Callback {
			override fun onFailure(call: Call, e: IOException) {
				result.setValue(DataLoaderResult(null, null, LoaderStatus.ERROR_NETWORK, null))
			}

			override fun onResponse(call: Call, response: Response) {
				val bytes: ByteArray? = response.body?.bytes()
				if (response.isSuccessful && bytes != null) {
					result.setValue(
						DataLoaderResult(
							ByteBuffer.allocateDirect(bytes.size).put(bytes),
							response.header(HEADER_NAME_ETAG, null),
							LoaderStatus.OK,
							null
						)
					)
				} else if (response.code == 404) {
					result.setValue(DataLoaderResult(null, null, LoaderStatus.ERROR_404, response.code.toString()))
				} else {
					result.setValue(DataLoaderResult(null, null, LoaderStatus.ERROR_OTHER, response.code.toString()))
				}
			}
		})
        return result.future
	}

	override fun cancel(url: String) {
		okHttpClient.dispatcher.queuedCalls().filter { it.request().tag() == url }.forEach { it.cancel() }
		okHttpClient.dispatcher.runningCalls().filter { it.request().tag() == url }.forEach { it.cancel() }
	}

	private fun initializeClient() = createClient()
}
