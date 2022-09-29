/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

package io.openmobilemaps.gps.providers

import android.location.Location
import com.google.android.gms.location.LocationRequest

interface LocationUpdateListener {
	val preferredPriority: Int
		get() = DEFAULT_PRIORITY

	val preferredInterval: Long
		get() = DEFAULT_INTERVAL

	fun onLocationUpdate(newLocation: Location)

	companion object {
		internal const val DEFAULT_PRIORITY = LocationRequest.PRIORITY_HIGH_ACCURACY
		internal const val DEFAULT_INTERVAL = 15 * 1000L
	}
}