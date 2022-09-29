/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

package io.openmobilemaps.gps.util

open class SingletonHolder<out T: Any, in A>(creator: (A) -> T) {

	private var creator: ((A) -> T)? = creator
	@Volatile private var instance: T? = null

	fun getInstance(arg: A): T {
		val i = instance
		if (i != null) {
			return i
		}

		return synchronized(this) {
			val i2 = instance
			if (i2 != null) {
				i2
			} else {
				val created = creator!!(arg)
				instance = created
				creator = null
				created
			}
		}
	}
}