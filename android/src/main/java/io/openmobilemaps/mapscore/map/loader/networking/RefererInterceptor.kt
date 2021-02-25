/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

package io.openmobilemaps.mapscore.map.loader.networking

import android.os.Build
import okhttp3.Interceptor
import okhttp3.Response

class RefererInterceptor @JvmOverloads constructor(val referer: String) : Interceptor {

	override fun intercept(chain: Interceptor.Chain): Response {
		val request = chain.request()
			.newBuilder()
			.header("referer", referer)
			.build()
		return chain.proceed(request)
	}

}