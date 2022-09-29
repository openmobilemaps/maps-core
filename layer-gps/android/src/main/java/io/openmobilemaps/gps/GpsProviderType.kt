/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

package io.openmobilemaps.gps

import android.content.Context
import io.openmobilemaps.gps.providers.GoogleFusedLocationProvider
import io.openmobilemaps.gps.providers.GpsOnlyLocationProvider
import io.openmobilemaps.gps.providers.LocationProviderInterface

enum class GpsProviderType {
	GPS_ONLY,
	GOOGLE_FUSED;

	fun getProvider(context: Context) : LocationProviderInterface = when(this) {
		GPS_ONLY -> GpsOnlyLocationProvider.getInstance(context)
		GOOGLE_FUSED -> GoogleFusedLocationProvider.getInstance(context)
	}
}