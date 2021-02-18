/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

package ch.ubique.mapscore.shared.map.view

import android.content.Context
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import android.view.MotionEvent
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleObserver
import androidx.lifecycle.OnLifecycleEvent
import androidx.lifecycle.coroutineScope
import ch.ubique.mapscore.shared.graphics.GlTextureView
import ch.ubique.mapscore.shared.graphics.common.Color
import ch.ubique.mapscore.shared.graphics.common.Vec2F
import ch.ubique.mapscore.shared.graphics.common.Vec2I
import ch.ubique.mapscore.shared.map.*
import ch.ubique.mapscore.shared.map.controls.TouchAction
import ch.ubique.mapscore.shared.map.controls.TouchEvent
import ch.ubique.mapscore.shared.map.controls.TouchHandlerInterface
import ch.ubique.mapscore.shared.map.scheduling.AndroidScheduler
import ch.ubique.mapscore.shared.map.scheduling.AndroidSchedulerCallback
import ch.ubique.mapscore.shared.map.scheduling.TaskInterface
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class MapView @JvmOverloads constructor(context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0) :
	GlTextureView(context, attrs, defStyleAttr), GLSurfaceView.Renderer, AndroidSchedulerCallback, LifecycleObserver {

	private lateinit var mapInterface: MapInterface
	private lateinit var scheduler: AndroidScheduler

	private lateinit var touchHandler: TouchHandlerInterface
	private var touchDisabled = false

	init {
		System.loadLibrary("mapscore_shared")
	}

	fun setupMap(mapConfig: MapConfig) {
		val densityExact = resources.displayMetrics.xdpi

		setRenderer(this)
		scheduler = AndroidScheduler(this)
		mapInterface = MapInterface.createWithOpenGl(
			mapConfig,
			scheduler,
			densityExact
		)
		mapInterface.setCallbackHandler(object : MapCallbackInterface() {
			override fun invalidate() {
				requestRender()
			}
		})
		touchHandler = mapInterface.getTouchHandler()
		mapInterface.setBackgroundColor(Color(1f, 1f, 1f, 1f))
	}

	fun registerLifecycle(lifecycle: Lifecycle) {
		lifecycle.addObserver(this)
		scheduler.setCoroutineScope(lifecycle.coroutineScope)
	}

	override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
		mapInterface.getRenderingContext().onSurfaceCreated()
	}

	override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
		mapInterface.setViewportSize(Vec2I(width, height))
	}

	override fun onDrawFrame(gl: GL10?) {
		mapInterface.drawFrame()
	}

	override fun scheduleOnGlThread(task: TaskInterface) {
		queueEvent { task.run() }
	}

	@OnLifecycleEvent(Lifecycle.Event.ON_START)
	fun onStart() {
		scheduler.resume()
	}

	@OnLifecycleEvent(Lifecycle.Event.ON_RESUME)
	fun onResume() {
		mapInterface.resume()
	}

	@OnLifecycleEvent(Lifecycle.Event.ON_PAUSE)
	fun onPause() {
		mapInterface.pause()
	}

	@OnLifecycleEvent(Lifecycle.Event.ON_STOP)
	fun onStop() {
		scheduler.pause()
	}

	fun setTouchEnabled(enabled: Boolean) {
		touchDisabled = !enabled
	}

	override fun onTouchEvent(event: MotionEvent?): Boolean {
		if (event == null || touchDisabled) {
			return false
		}

		val action: TouchAction? = when (event.actionMasked) {
			MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> TouchAction.DOWN
			MotionEvent.ACTION_MOVE -> TouchAction.MOVE
			MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP -> TouchAction.UP
			else -> null
		}

		if (action != null) {
			val pointers = ArrayList<Vec2F>(10)
			for (i in 0 until event.pointerCount) {
				pointers.add(Vec2F(event.getX(i), event.getY(i)))
			}
			touchHandler.onTouchEvent(TouchEvent(pointers, action))
		}

		return true
	}

	fun setBackgroundColor(color: Color) {
		mapInterface.setBackgroundColor(color)
	}

	fun addLayer(layer: LayerInterface) {
		mapInterface.addLayer(layer)
	}

	fun insertLayerAt(layer: LayerInterface, at: Int) {
		mapInterface.insertLayerAt(layer, at)
	}

	fun insertLayerAbove(layer: LayerInterface, above: LayerInterface) {
		mapInterface.insertLayerAbove(layer, above)
	}

	fun insertLayerBelow(layer: LayerInterface, below: LayerInterface) {
		mapInterface.insertLayerBelow(layer, below)
	}

	fun removeLayer(layer: LayerInterface) {
		mapInterface.removeLayer(layer)
	}

	fun getCamera() : MapCamera2dInterface {
		return mapInterface.getCamera()
	}

}