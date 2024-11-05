package io.openmobilemaps.mapscore.map.loader.networking

import okhttp3.CacheControl
import okhttp3.Interceptor
import okhttp3.Response
import java.util.concurrent.TimeUnit

class CacheControlResponseNetworkInterceptor(val cacheControl: CacheControl) : Interceptor {

	constructor(maxAgeSeconds: Int) : this(CacheControl.Builder().maxAge(maxAgeSeconds, TimeUnit.SECONDS).build())

	override fun intercept(chain: Interceptor.Chain): Response {
		val response = chain.proceed(chain.request())
		return response.newBuilder()
			.header("Cache-Control", cacheControl.toString())
			.build()
	}
}