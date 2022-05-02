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
import android.graphics.Bitmap
import android.opengl.GLES20
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import android.view.MotionEvent
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleObserver
import androidx.lifecycle.OnLifecycleEvent
import androidx.lifecycle.coroutineScope
import io.openmobilemaps.mapscore.graphics.GlTextureView
import io.openmobilemaps.mapscore.map.scheduling.AndroidScheduler
import io.openmobilemaps.mapscore.map.scheduling.AndroidSchedulerCallback
import io.openmobilemaps.mapscore.shared.graphics.common.Color
import io.openmobilemaps.mapscore.shared.graphics.common.Vec2F
import io.openmobilemaps.mapscore.shared.graphics.common.Vec2I
import io.openmobilemaps.mapscore.shared.map.*
import io.openmobilemaps.mapscore.shared.map.controls.TouchAction
import io.openmobilemaps.mapscore.shared.map.controls.TouchEvent
import io.openmobilemaps.mapscore.shared.map.controls.TouchHandlerInterface
import io.openmobilemaps.mapscore.shared.map.scheduling.TaskInterface
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.ShortBuffer
import java.util.concurrent.atomic.AtomicBoolean
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

open class MapView @JvmOverloads constructor(context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0) :
	GlTextureView(context, attrs, defStyleAttr), GLSurfaceView.Renderer, AndroidSchedulerCallback, LifecycleObserver {

	var mapInterface: MapInterface? = null
		private set
	protected var scheduler: AndroidScheduler? = null
		private set

	private var touchHandler: TouchHandlerInterface? = null
	private var touchDisabled = false

	private val saveFrame: AtomicBoolean = AtomicBoolean(false)
	private var saveFrameSpec: SaveFrameSpec? = null
	private var saveFrameCallback: SaveFrameCallback? = null

	open fun setupMap(mapConfig: MapConfig, useMSAA: Boolean = false) {
		val densityExact = resources.displayMetrics.xdpi

		configureGL(useMSAA)
		setRenderer(this)
		val scheduler = AndroidScheduler(this)
		val mapInterface = MapInterface.createWithOpenGl(
			mapConfig,
			scheduler,
			densityExact
		)
		mapInterface.setCallbackHandler(object : MapCallbackInterface() {
			override fun invalidate() {
				scheduler.launchCoroutine { requestRender() }
			}
		})
		mapInterface.setBackgroundColor(Color(1f, 1f, 1f, 1f))
		touchHandler = mapInterface.getTouchHandler()
		this.mapInterface = mapInterface
		this.scheduler = scheduler
	}

	fun registerLifecycle(lifecycle: Lifecycle) {
		requireScheduler().setCoroutineScope(lifecycle.coroutineScope)
		lifecycle.addObserver(this)
	}

	override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
		requireMapInterface().getRenderingContext().onSurfaceCreated()
	}

	override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
		requireMapInterface().setViewportSize(Vec2I(width, height))
	}

	override fun onDrawFrame(gl: GL10?) {
		mapInterface?.drawFrame()
		if (saveFrame.getAndSet(false)) {
			saveFrame()
		}
	}

	override fun scheduleOnGlThread(task: TaskInterface) {
		queueEvent { task.run() }
	}

	@OnLifecycleEvent(Lifecycle.Event.ON_START)
	open fun onStart() {
		requireScheduler().resume()
	}

	@OnLifecycleEvent(Lifecycle.Event.ON_RESUME)
	open fun onResume() {
		requireMapInterface().resume()
	}

	@OnLifecycleEvent(Lifecycle.Event.ON_PAUSE)
	open fun onPause() {
		requireMapInterface().pause()
	}

	@OnLifecycleEvent(Lifecycle.Event.ON_STOP)
	open fun onStop() {
		requireScheduler().pause()
	}

	@OnLifecycleEvent(Lifecycle.Event.ON_DESTROY)
	open fun onDestroy() {
		setRenderer(null)
		mapInterface = null
		scheduler = null
		touchHandler = null
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
			touchHandler?.onTouchEvent(TouchEvent(pointers, action))
		}

		return true
	}

	fun setBackgroundColor(color: Color) {
		requireMapInterface().setBackgroundColor(color)
	}

	open fun addLayer(layer: LayerInterface) {
		requireMapInterface().addLayer(layer)
	}

	open fun insertLayerAt(layer: LayerInterface, at: Int) {
		requireMapInterface().insertLayerAt(layer, at)
	}

	open fun insertLayerAbove(layer: LayerInterface, above: LayerInterface) {
		requireMapInterface().insertLayerAbove(layer, above)
	}

	open fun insertLayerBelow(layer: LayerInterface, below: LayerInterface) {
		requireMapInterface().insertLayerBelow(layer, below)
	}

	open fun removeLayer(layer: LayerInterface) {
		requireMapInterface().removeLayer(layer)
	}

	fun getCamera(): MapCamera2dInterface {
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

		try {
			val left = (spec.cropLeftPc * width).toInt()
			val right = ((1f - spec.cropRightPc) * width).toInt()
			val top = ((1f - spec.cropTopPc) * height).toInt()
			val bottom = (spec.cropBottomPc * height).toInt()

			val width: Int = right - left
			val height: Int = top - bottom
			val screenshotSize = width * height
			val bb = ByteBuffer.allocateDirect(screenshotSize * 4)
			bb.order(ByteOrder.nativeOrder())
			GLES20.glReadPixels(left, bottom, width, height, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, bb)
			val pixelsBuffer = IntArray(screenshotSize)
			bb.asIntBuffer()[pixelsBuffer]
			val bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.RGB_565)
			bitmap.setPixels(pixelsBuffer, screenshotSize - width, -width, 0, 0, width, height)
			val scaledBitmap = Bitmap.createScaledBitmap(bitmap, spec.targetWidthPx ?: width, spec.targetHeightPx ?: height, false)

			val sBuffer = ShortArray(screenshotSize)
			val sb = ShortBuffer.wrap(sBuffer)
			scaledBitmap.copyPixelsToBuffer(sb)

			for (i in 0 until screenshotSize) {
				val v: Long = sBuffer[i].toLong()
				sBuffer[i] = (((v and 0x1f) shl 11) or (v and 0x7e0) or ((v and 0xf800) shr 11)).toShort()
			}
			sb.rewind()
			scaledBitmap.copyPixelsFromBuffer(sb)

			val size: Int = scaledBitmap.rowBytes * scaledBitmap.height

			val b = ByteBuffer.allocate(size)

			scaledBitmap.copyPixelsToBuffer(b)
			b.rewind()
			val bytes = ByteArray(size)
			b.get(bytes)
			post {
				saveFrameSpec = null
				saveFrameCallback?.onResult(bytes)
				saveFrameCallback = null
			}
		} catch (e: Exception) {
			post {
				saveFrameSpec = null
				saveFrameCallback?.onError()
				saveFrameCallback = null
			}
		}
	}

	fun requireMapInterface(): MapInterface = mapInterface ?: throw IllegalStateException("Map is not setup or already destroyed!")
	fun requireScheduler(): AndroidScheduler = scheduler ?: throw IllegalStateException("Map is not setup or already destroyed!")

	data class SaveFrameSpec(
		val targetWidthPx: Int? = null,
		val targetHeightPx: Int? = null,
		val cropLeftPc: Float = 0f,
		val cropTopPc: Float = 0f,
		val cropRightPc: Float = 0f,
		val cropBottomPc: Float = 0f
	)

	interface SaveFrameCallback {
		fun onResult(thumbnail: ByteArray)
		fun onError()
	}
}