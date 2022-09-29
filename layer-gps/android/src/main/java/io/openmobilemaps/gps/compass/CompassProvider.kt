/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

package io.openmobilemaps.gps.compass

import android.content.Context
import android.hardware.Sensor
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.hardware.SensorEvent
import android.view.Display
import android.view.WindowManager
import io.openmobilemaps.gps.util.SingletonHolder
import kotlin.collections.HashSet

class CompassProvider private constructor(context: Context) : SensorEventListener {

	companion object : SingletonHolder<CompassProvider, Context>(::CompassProvider);

	private val sensorManager: SensorManager = context.getSystemService(Context.SENSOR_SERVICE) as SensorManager
	private val accelerometer: Sensor? = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER)
	private val magnetometer: Sensor? = sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD)
	private val display: Display = (context.getSystemService(Context.WINDOW_SERVICE) as WindowManager).defaultDisplay
	private val R = FloatArray(9)
	private val I = FloatArray(9)
	private val direction = Vector()
	private val v = Vector()
	private var gravity: FloatArray? = null
	private var geomagnetic: FloatArray? = null

	private val compassUpdateListeners: MutableSet<CompassUpdateListener> = HashSet()

	override fun onAccuracyChanged(sensor: Sensor, accuracy: Int) {}

	override fun onSensorChanged(event: SensorEvent) {
		if (event.sensor.type == Sensor.TYPE_ACCELEROMETER) {
			gravity = event.values
		}
		if (event.sensor.type == Sensor.TYPE_MAGNETIC_FIELD) {
			geomagnetic = event.values
		}
		if (gravity == null || geomagnetic == null || !SensorManager.getRotationMatrix(R, I, gravity, geomagnetic)) {
			return
		}
		val orientation = FloatArray(3)
		SensorManager.getOrientation(R, orientation)
		val angle = orientation[0] + display.rotation * Math.PI / 2
		val degrees = (getSmoothedAngle(angle) * 180 / Math.PI).toFloat()

		compassUpdateListeners.forEach { it.onCompassUpdate(degrees) }
	}

	/**
	 * Get the temporally smoothed angle. The larger the offset of the new value is, the more weight it gets.
	 * Small changes in angle are thus basically ignored.
	 */
	private fun getSmoothedAngle(angle: Double): Double {
		v[Math.cos(angle)] = Math.sin(angle)
		if (direction.length() == 0.0) {
			direction.set(v)
		}
		var w = Math.acos((v.x * direction.x + v.y * direction.y) / (direction.length() * v.length())) / Math.PI
		w = Math.min(w * w, 0.05)
		direction.x = direction.x * (1 - w) + v.x * w
		direction.y = direction.y * (1 - w) + v.y * w
		direction.normalize()
		if (direction.isInvalid) {
			direction.set(v)
		}
		return Math.atan2(direction.y, direction.x)
	}

	fun registerCompassUpdateListener(compassUpdateListener: CompassUpdateListener) {
		val emptyBefore = compassUpdateListeners.isEmpty()
		compassUpdateListeners.add(compassUpdateListener)
		if (emptyBefore) onActive()
	}

	fun unregisterCompassUpdateListener(compassUpdateListener: CompassUpdateListener) {
		compassUpdateListeners.remove(compassUpdateListener)
		if (compassUpdateListeners.isEmpty()) {
			onInactive()
		}
	}

	fun onActive() {
		accelerometer?.let { sensorManager.registerListener(this, it, SensorManager.SENSOR_DELAY_UI) }
		magnetometer?.let { sensorManager.registerListener(this, it, SensorManager.SENSOR_DELAY_UI) }
	}

	fun onInactive() {
		sensorManager.unregisterListener(this)
		gravity = null
		geomagnetic = null
	}


	private class Vector {
		var x = 0.0
		var y = 0.0
		fun length(): Double {
			return Math.sqrt(x * x + y * y)
		}

		operator fun set(xn: Double, yn: Double) {
			x = xn
			y = yn
		}

		fun set(v: Vector) {
			x = v.x
			y = v.y
		}

		val isInvalid: Boolean
			get() = java.lang.Double.isNaN(x) || java.lang.Double.isNaN(y) || java.lang.Double.isInfinite(x) || java.lang.Double.isInfinite(
				y
			)

		fun normalize() {
			val l = length()
			x /= l
			y /= l
		}
	}

}