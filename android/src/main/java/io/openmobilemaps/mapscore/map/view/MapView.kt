/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

package io.openmobilemaps.mapscore.map.view

import android.content.Context
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import android.view.MotionEvent
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleObserver
import androidx.lifecycle.OnLifecycleEvent
import io.openmobilemaps.mapscore.graphics.GlTextureView
import io.openmobilemaps.mapscore.map.layers.TiledRasterLayer
import io.openmobilemaps.mapscore.map.layers.TiledVectorLayer
import io.openmobilemaps.mapscore.map.scheduling.AndroidSchedulerCallback
import io.openmobilemaps.mapscore.map.util.MapViewInterface
import io.openmobilemaps.mapscore.map.util.SaveFrameCallback
import io.openmobilemaps.mapscore.map.util.SaveFrameSpec
import io.openmobilemaps.mapscore.map.util.SaveFrameUtil
import io.openmobilemaps.mapscore.shared.graphics.common.Color
import io.openmobilemaps.mapscore.shared.graphics.common.Vec2F
import io.openmobilemaps.mapscore.shared.graphics.common.Vec2I
import io.openmobilemaps.mapscore.shared.map.*
import io.openmobilemaps.mapscore.shared.map.controls.TouchAction
import io.openmobilemaps.mapscore.shared.map.controls.TouchEvent
import io.openmobilemaps.mapscore.shared.map.controls.TouchHandlerInterface
import io.openmobilemaps.mapscore.shared.map.scheduling.TaskInterface
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import java.util.*
import java.util.concurrent.atomic.AtomicBoolean
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

open class MapView @JvmOverloads constructor(context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0) :
	GlTextureView(context, attrs, defStyleAttr), GLSurfaceView.Renderer, AndroidSchedulerCallback, LifecycleObserver,
	MapViewInterface {

	var mapInterface: MapInterface? = null
		private set

	private var touchHandler: TouchHandlerInterface? = null
	private var touchDisabled = false

	private val saveFrame: AtomicBoolean = AtomicBoolean(false)
	private var saveFrameSpec: SaveFrameSpec? = null
	private var saveFrameCallback: SaveFrameCallback? = null

	protected var lifecycle: Lifecycle? = null
	private var lifecycleResumed = false
	private val mapViewStateMutable = MutableStateFlow(MapViewState.UNINITIALIZED)
	val mapViewState = mapViewStateMutable.asStateFlow()

	open fun setupMap(mapConfig: MapConfig, density: Float = resources.displayMetrics.xdpi, useMSAA: Boolean = false) {
		configureGL(useMSAA)
		setRenderer(this)
		val mapInterface = MapInterface.createWithOpenGl(
			mapConfig,
			density
		)
		mapInterface.setCallbackHandler(object : MapCallbackInterface() {
			override fun invalidate() {
				requestRender()
			}

			override fun onMapResumed() {
				updateMapViewState(MapViewState.RESUMED)
			}
		})
		mapInterface.setBackgroundColor(Color(1f, 1f, 1f, 1f))
		touchHandler = mapInterface.getTouchHandler()
		this.mapInterface = mapInterface
		updateMapViewState(MapViewState.INITIALIZED)
	}

	fun registerLifecycle(lifecycle: Lifecycle) {
		lifecycle.addObserver(this)
		this.lifecycle = lifecycle
	}

	override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
		requireMapInterface().getRenderingContext().onSurfaceCreated()
	}

	override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
		requireMapInterface().setViewportSize(Vec2I(width, height))
	}

	override fun onDrawFrame(gl: GL10?) {
		mapInterface?.apply {
			prepare()
			compute()
			drawFrame()
		}
		if (saveFrame.getAndSet(false)) {
			saveFrame()
		}
	}

	override fun scheduleOnGlThread(task: TaskInterface) {
		queueEvent { task.run() }
	}

	@OnLifecycleEvent(Lifecycle.Event.ON_RESUME)
	open fun onResume() {
		lifecycleResumed = true
		resumeGlThread()
	}

	@OnLifecycleEvent(Lifecycle.Event.ON_PAUSE)
	open fun onPause() {
		lifecycleResumed = false
		pauseGlThread()
	}

	@OnLifecycleEvent(Lifecycle.Event.ON_DESTROY)
	open fun onDestroy() {
		lifecycle = null
		updateMapViewState(MapViewState.DESTROYED)
		finishGlThread()
	}

	override fun onGlThreadResume() {
		if (lifecycleResumed) {
			requireMapInterface().resume()
		}
	}

	override fun onGlThreadPause() {
		if (mapViewStateMutable.value != MapViewState.PAUSED) {
			updateMapViewState(MapViewState.PAUSED)
			requireMapInterface().pause()
		}
	}

	override fun onGlThreadFinishing() {
		if (mapViewState.value == MapViewState.DESTROYED) {
			val map = mapInterface
			setRenderer(null)
			mapInterface = null
			touchHandler = null
			map?.destroy()
		}
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
			MotionEvent.ACTION_CANCEL -> TouchAction.CANCEL
			else -> null
		}

		if (action != null) {
			val pointers = ArrayList<Vec2F>(10)
			for (i in 0 until event.pointerCount) {
				pointers.add(Vec2F(event.getX(i), event.getY(i)))
			}
			touchHandler?.onTouchEvent(TouchEvent(pointers, action))
		}

		return true
	}

	override fun setBackgroundColor(color: Color) {
		requireMapInterface().setBackgroundColor(color)
	}

	override fun addLayer(layer: LayerInterface) {
		requireMapInterface().addLayer(layer)
	}

	fun addLayer(layer: TiledRasterLayer) {
		requireMapInterface().addLayer(layer.layerInterface())
	}

	fun addLayer(layer: TiledVectorLayer) {
		requireMapInterface().addLayer(layer.layerInterface())
	}

	override fun insertLayerAt(layer: LayerInterface, at: Int) {
		requireMapInterface().insertLayerAt(layer, at)
	}

	fun insertLayerAt(layer: TiledRasterLayer, at: Int) {
		requireMapInterface().insertLayerAt(layer.layerInterface(), at)
	}

	fun insertLayerAt(layer: TiledVectorLayer, at: Int) {
		requireMapInterface().insertLayerAt(layer.layerInterface(), at)
	}

	override fun insertLayerAbove(layer: LayerInterface, above: LayerInterface) {
		requireMapInterface().insertLayerAbove(layer, above)
	}

	override fun insertLayerBelow(layer: LayerInterface, below: LayerInterface) {
		requireMapInterface().insertLayerBelow(layer, below)
	}

	override fun removeLayer(layer: LayerInterface) {
		requireMapInterface().removeLayer(layer)
	}

	fun removeLayer(layer: TiledRasterLayer) {
		requireMapInterface().removeLayer(layer.layerInterface())
	}

	fun removeLayer(layer: TiledVectorLayer) {
		requireMapInterface().removeLayer(layer.layerInterface())
	}

	override fun getCamera(): MapCamera2dInterface {
		return requireMapInterface().getCamera()
	}

	fun saveFrame(saveFrameSpec: SaveFrameSpec, saveFrameCallback: SaveFrameCallback) {
		this.saveFrameSpec = saveFrameSpec
		this.saveFrameCallback = saveFrameCallback
		saveFrame.set(true)
		invalidate()
	}

	private fun saveFrame() {
		val callback = saveFrameCallback ?: return
		val spec = saveFrameSpec ?: return
		saveFrameCallback = null
		saveFrameSpec = null
		SaveFrameUtil.saveCurrentFrame(Vec2I(width, height), spec, callback)
	}

	private fun updateMapViewState(state: MapViewState) {
		mapViewStateMutable.value = state
		onMapViewStateUpdated(state)
	}

	protected open fun onMapViewStateUpdated(state: MapViewState) {}

	override fun requireMapInterface(): MapInterface = mapInterface ?: throw IllegalStateException("Map is not setup or already destroyed!")
}