package io.openmobilemaps.mapscore.map.util

import android.opengl.GLSurfaceView
import io.openmobilemaps.mapscore.graphics.GLThread
import io.openmobilemaps.mapscore.map.scheduling.AndroidSchedulerCallback
import io.openmobilemaps.mapscore.shared.graphics.common.Color
import io.openmobilemaps.mapscore.shared.graphics.common.Vec2I
import io.openmobilemaps.mapscore.shared.map.*
import io.openmobilemaps.mapscore.shared.map.coordinates.CoordinateConversionHelperInterface
import io.openmobilemaps.mapscore.shared.map.scheduling.TaskInterface
import io.openmobilemaps.mapscore.shared.map.scheduling.ThreadPoolScheduler
import kotlinx.coroutines.CoroutineScope
import java.util.concurrent.atomic.AtomicBoolean
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10


open class OffscreenMapRenderer(val sizePx: Vec2I, val density: Float = 72f) : GLSurfaceView.Renderer, AndroidSchedulerCallback,
	MapViewInterface {

	private lateinit var glThread: GLThread

	var mapInterface: MapInterface? = null
		private set

	private val saveFrame: AtomicBoolean = AtomicBoolean(false)
	private var saveFrameSpec: SaveFrameSpec? = null
	private var saveFrameCallback: SaveFrameCallback? = null

	open fun setupMap(coroutineScope: CoroutineScope, mapConfig: MapConfig, useMSAA: Boolean = false) {
		val scheduler = ThreadPoolScheduler.create()
		val mapInterface = MapInterface.createWithOpenGl(
			mapConfig,
			scheduler,
			density,
			false
		)
		mapInterface.setCallbackHandler(object : MapCallbackInterface() {
			override fun invalidate() {
				glThread.requestRender()
			}

			override fun onMapResumed() {
				// not used
			}
		})
		mapInterface.setBackgroundColor(Color(1f, 1f, 1f, 1f))
		this.mapInterface = mapInterface

		glThread = GLThread(onResumeCallback = this::onGlThreadResume,
			onPauseCallback = this::onGlThreadPause,
			onFinishingCallback = this::onGlThreadFinishing).apply {
			this.useMSAA = useMSAA
			onWindowResize(sizePx.x, sizePx.y)
			renderer = this@OffscreenMapRenderer
			doResume()
			start()
		}
	}

	protected open fun onGlThreadFinishing() {
		glThread.renderer = null
		mapInterface?.destroy()
		mapInterface = null
	}

	protected open fun onGlThreadPause() {
		requireMapInterface().pause()
	}

	protected open fun onGlThreadResume() {
		requireMapInterface().resume()
	}

	fun setOnDrawCallback(onDrawCallback: (() -> Unit)? = null) {
		glThread.onDrawCallback = onDrawCallback
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
		glThread.queueEvent { task.run() }
	}

	fun resume() {
		glThread.doResume()
	}

	fun destroy() {
		glThread.doPause()
		glThread.finish()
	}

	override fun setBackgroundColor(color: Color) {
		requireMapInterface().setBackgroundColor(color)
	}

	override fun addLayer(layer: LayerInterface) {
		requireMapInterface().addLayer(layer)
	}

	override fun insertLayerAt(layer: LayerInterface, at: Int) {
		requireMapInterface().insertLayerAt(layer, at)
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

	override fun getCamera(): MapCameraInterface {
		return requireMapInterface().getCamera()
	}

	override fun getCoordinateConversionHelper(): CoordinateConversionHelperInterface {
		return requireMapInterface().getCoordinateConverterHelper()
	}

	fun saveFrame(saveFrameSpec: SaveFrameSpec, saveFrameCallback: SaveFrameCallback) {
		this.saveFrameSpec = saveFrameSpec
		this.saveFrameCallback = saveFrameCallback
		saveFrame.set(true)
	}

	private fun saveFrame() {
		val callback = saveFrameCallback ?: return
		val spec = saveFrameSpec ?: return
		saveFrameCallback = null
		saveFrameSpec = null
		SaveFrameUtil.saveCurrentFrame(sizePx, spec, callback)
	}

	override fun requireMapInterface(): MapInterface = mapInterface ?: throw IllegalStateException("Map is not setup or already destroyed!")
}
