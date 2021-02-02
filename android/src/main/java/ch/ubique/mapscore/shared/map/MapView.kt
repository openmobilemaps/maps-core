package ch.ubique.mapscore.shared.map

import android.content.Context
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleObserver
import androidx.lifecycle.OnLifecycleEvent
import androidx.lifecycle.coroutineScope
import ch.ubique.mapscore.shared.graphics.GlTextureView
import ch.ubique.mapscore.shared.graphics.SceneInterface
import ch.ubique.mapscore.shared.map.scheduling.AndroidScheduler
import ch.ubique.mapscore.shared.map.scheduling.AndroidSchedulerCallback
import ch.ubique.mapscore.shared.map.scheduling.TaskInterface
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class MapView @JvmOverloads constructor(context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0) :
	GlTextureView(context, attrs, defStyleAttr), GLSurfaceView.Renderer, AndroidSchedulerCallback, LifecycleObserver {

	//lateinit var mapInterface: MapInterface
	var scene: SceneInterface = SceneInterface.createWithOpenGl()
	var scheduler: AndroidScheduler

	init {
		setRenderer(this)
		scheduler = AndroidScheduler(this)
	}

	fun registerLifecycle(lifecycle: Lifecycle) {
		lifecycle.addObserver(this)
		scheduler.setCoroutineScope(lifecycle.coroutineScope)
	}

	override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
		scene.renderingContext.onSurfaceCreated()
	}

	override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
		scene.renderingContext.viewportSize
	}

	override fun onDrawFrame(gl: GL10?) {
		scene.drawFrame()
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
		//mapInterface.pause()
	}

	@OnLifecycleEvent(Lifecycle.Event.ON_PAUSE)
	fun onPause() {
		//mapInterface.resume()
	}

	@OnLifecycleEvent(Lifecycle.Event.ON_STOP)
	fun onStop() {
		scheduler.pause()
	}

}