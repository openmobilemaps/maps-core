package ch.ubique.mapscore.shared.graphics

import android.content.Context
import android.graphics.SurfaceTexture
import android.opengl.GLSurfaceView
import android.opengl.GLUtils
import android.util.AttributeSet
import android.util.Log
import android.view.TextureView
import android.view.TextureView.SurfaceTextureListener
import java.util.concurrent.ConcurrentLinkedQueue
import java.util.concurrent.atomic.AtomicBoolean
import javax.microedition.khronos.egl.*
import javax.microedition.khronos.opengles.GL
import javax.microedition.khronos.opengles.GL10


open class GlTextureView @JvmOverloads constructor(context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0) :
	TextureView(
		context, attrs, defStyleAttr
	), SurfaceTextureListener {

	companion object {
		private const val TAG = "GLTextureView"
		const val EGL_CONTEXT_CLIENT_VERSION = 0x3098
		const val EGL_OPENGL_ES2_BIT = 4
	}

	init {
		surfaceTextureListener = this
	}

	private var beforeRenderTimes = LongArray(50)
	private var renderTimes = LongArray(50)
	private var currentTimeRec = 0

	private var glThread: GLThread? = null
	private var renderer: GLSurfaceView.Renderer? = null

	var glRunList = ConcurrentLinkedQueue<() -> Unit>()
	private val runNotifier = Object()

	private val isDirty = AtomicBoolean(false)
	private var frameRate = -1

	override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
		glThread = GLThread(surface).apply { start() }
	}

	override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {
		glThread?.onWindowResize(width, height)
	}

	override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
		glThread?.finish()
		return false
	}

	override fun onSurfaceTextureUpdated(surface: SurfaceTexture) {}

	fun setRenderer(renderer: GLSurfaceView.Renderer) {
		this.renderer = renderer
	}

	fun requestRender() {
		isDirty.set(true)
		synchronized(runNotifier) { runNotifier.notify() }
	}

	fun queueEvent(clearQueueIfNotRunning: Boolean = false, r: () -> Unit) {
		if (clearQueueIfNotRunning && (glThread == null || glThread?.isAlive != true)) {
			glRunList.clear()
		} else {
			glRunList.add(r)
			requestRender()
		}
	}

	fun setRenderFrameRate(frameRate: Int) {
		this.frameRate = frameRate
	}

	private inner class GLThread internal constructor(private val surface: SurfaceTexture) : Thread("GLThread") {
		@Volatile
		private var finished = false
		private var egl: EGL10? = null
		private var eglDisplay: EGLDisplay? = null
		private var eglConfig: EGLConfig? = null
		private var eglContext: EGLContext? = null
		private var eglSurface: EGLSurface? = null
		private var gl: GL? = null
		private var width = getWidth()
		private var height = getHeight()

		@Volatile
		private var sizeChanged = true
		override fun run() {
			initGL()
			val gl10 = gl as GL10?
			val renderer = renderer ?: throw IllegalStateException("No renderer attached to GlTextureView")
			renderer.onSurfaceCreated(gl10, eglConfig)
			while (!finished) {
				val timestampStartRender = System.currentTimeMillis()
				checkCurrent()
				if (sizeChanged) {
					createSurface()
					renderer.onSurfaceChanged(gl10, width, height)
					sizeChanged = false
				}
				var i = 0
				while (glRunList.isNotEmpty() && i < 4) {
					glRunList.poll()?.invoke()
					i++
				}

				beforeRenderTimes[currentTimeRec] = timestampStartRender - System.currentTimeMillis()
				val startDraw = System.currentTimeMillis()

				renderer.onDrawFrame(gl10)

				val afterDraw = System.currentTimeMillis()
				renderTimes[currentTimeRec] = afterDraw - startDraw
				currentTimeRec = (currentTimeRec + 1) % renderTimes.size
				beforeRenderTimes[currentTimeRec] = System.currentTimeMillis()
				val avg = renderTimes.filter { l -> l > 0 }.average()
				val avgBetween = beforeRenderTimes.filter { l -> l > 0 }.average()
				Log.d("GLTEXTURE", "Avg. render time: $avg ms")
				Log.d("GLTEXTURE", "Avg. before render time: $avgBetween ms")

				if (egl?.eglSwapBuffers(eglDisplay, eglSurface) != true) {
					throw RuntimeException("Cannot swap buffers")
				}
				if (frameRate > 0) {
					val renderTime = System.currentTimeMillis() - timestampStartRender
					try {
						val sleepTime = Math.max(0, 1000 / Math.min(1000, frameRate) - renderTime)
						if (sleepTime > 0) sleep(sleepTime)
					} catch (e: InterruptedException) {
						// Ignore
					}
				}
				if (!isDirty.get()) {
					try {
						synchronized(runNotifier) { runNotifier.wait(1000) }
					} catch (e: InterruptedException) {
						e.printStackTrace()
					}
				}
				if (glRunList.isEmpty()) {
					isDirty.set(false)
				}
			}
			finishGL()
		}

		private fun destroySurface() {
			if (eglSurface != null && eglSurface !== EGL10.EGL_NO_SURFACE) {
				egl?.eglMakeCurrent(eglDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT)
				egl?.eglDestroySurface(eglDisplay, eglSurface)
				eglSurface = null
			}
		}

		/**
		 * Create an egl surface for the current SurfaceHolder surface. If a surface
		 * already exists, destroy it before creating the new surface.
		 * @return true if the surface was created successfully.
		 */
		fun createSurface(): Boolean {
			/*
			 * Check preconditions.
			 */
			if (egl == null) {
				throw RuntimeException("egl not initialized")
			}
			if (eglDisplay == null) {
				throw RuntimeException("eglDisplay not initialized")
			}
			if (eglConfig == null) {
				throw RuntimeException("eglConfig not initialized")
			}

			/*
			 *  The window size has changed, so we need to create a new
			 *  surface.
			 */destroySurface()

			/*
			 * Create an EGL surface we can render into.
			 */eglSurface = try {
				egl?.eglCreateWindowSurface(eglDisplay, eglConfig, surface, null)
			} catch (e: IllegalArgumentException) {
				// This exception indicates that the surface flinger surface
				// is not valid. This can happen if the surface flinger surface has
				// been torn down, but the application has not yet been
				// notified via SurfaceHolder.Callback.surfaceDestroyed.
				// In theory the application should be notified first,
				// but in practice sometimes it is not. See b/4588890
				Log.e(TAG, "eglCreateWindowSurface", e)
				return false
			}
			if (eglSurface == null || eglSurface === EGL10.EGL_NO_SURFACE) {
				val error = egl!!.eglGetError()
				if (error == EGL10.EGL_BAD_NATIVE_WINDOW) {
					Log.e(TAG, "createWindowSurface returned EGL_BAD_NATIVE_WINDOW.")
				}
				return false
			}

			/*
			 * Before we can issue GL commands, we need to make sure
			 * the context is current and bound to a surface.
			 */if (egl?.eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext) != true) {
				/*
				 * Could not make the context current, probably because the underlying
				 * SurfaceView surface has been destroyed.
				 */
				Log.e(
					TAG,
					"eglMakeCurrent failed " + (egl?.let { GLUtils.getEGLErrorString(it.eglGetError()) } ?: " - egl is null"))
				return false
			}
			return true
		}

		private fun checkCurrent() {
			val egl = egl ?: throw IllegalStateException("EGL is null")
			if (eglContext != egl.eglGetCurrentContext() || eglSurface != egl.eglGetCurrentSurface(EGL10.EGL_DRAW)) {
				checkEglError()
				if (!egl.eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext)) {
					throw RuntimeException("eglMakeCurrent failed " + GLUtils.getEGLErrorString(egl.eglGetError()))
				}
				checkEglError()
			}
		}

		private fun checkEglError() {
			val egl = egl ?: throw IllegalStateException("CheckEglError: EGL is null")
			val error = egl.eglGetError()
			if (error != EGL10.EGL_SUCCESS) {
				Log.e("PanTextureView", "EGL error = 0x" + Integer.toHexString(error))
			}
		}

		private fun finishGL() {
			egl?.eglDestroyContext(eglDisplay, eglContext)
			egl?.eglTerminate(eglDisplay)
			egl?.eglDestroySurface(eglDisplay, eglSurface)
		}

		private fun initGL() {
			val egl = (EGLContext.getEGL() as EGL10)
			this.egl = egl
			eglDisplay = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY)
			if (eglDisplay === EGL10.EGL_NO_DISPLAY) {
				throw RuntimeException("eglGetDisplay failed " + GLUtils.getEGLErrorString(egl.eglGetError()))
			}
			val version = IntArray(2)
			if (!egl.eglInitialize(eglDisplay, version)) {
				throw RuntimeException("eglInitialize failed " + GLUtils.getEGLErrorString(egl.eglGetError()))
			}
			eglConfig = chooseEglConfig(egl)
			if (eglConfig == null) {
				throw RuntimeException("eglConfig not initialized")
			}
			eglContext = createContext(egl, eglDisplay, eglConfig)?.also { gl = it.gl }
			createSurface()
			if (!egl.eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext)) {
				throw RuntimeException("eglMakeCurrent failed " + GLUtils.getEGLErrorString(egl.eglGetError()))
			}
		}

		fun createContext(egl: EGL10, eglDisplay: EGLDisplay?, eglConfig: EGLConfig?): EGLContext? {
			val attrib_list = intArrayOf(EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE)
			return egl.eglCreateContext(eglDisplay, eglConfig, EGL10.EGL_NO_CONTEXT, attrib_list)
		}

		private fun chooseEglConfig(egl: EGL10): EGLConfig? {
			val configsCount = IntArray(1)
			val configs = arrayOfNulls<EGLConfig>(1)
			val configSpec = config
			require(egl.eglChooseConfig(eglDisplay, configSpec, configs, 1, configsCount)) {
				"eglChooseConfig failed " + GLUtils.getEGLErrorString(egl.eglGetError())
			}
			return if (configsCount[0] > 0) {
				configs[0]
			} else null
		}

		private val config: IntArray
			get() = intArrayOf(
				EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
				EGL10.EGL_RED_SIZE, 8,
				EGL10.EGL_GREEN_SIZE, 8,
				EGL10.EGL_BLUE_SIZE, 8,
				EGL10.EGL_ALPHA_SIZE, 8,
				EGL10.EGL_DEPTH_SIZE, 16,
				EGL10.EGL_STENCIL_SIZE, 8,
				EGL10.EGL_NONE
			)

		fun finish() {
			finished = true
			synchronized(runNotifier) { runNotifier.notify() }
		}

		@Synchronized
		fun onWindowResize(w: Int, h: Int) {
			width = w
			height = h
			sizeChanged = true
		}

	}
}