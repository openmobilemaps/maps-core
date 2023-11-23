/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

package io.openmobilemaps.mapscore.graphics

import android.graphics.SurfaceTexture
import android.opengl.*
import android.util.Log
import io.openmobilemaps.mapscore.BuildConfig
import java.util.concurrent.ConcurrentLinkedQueue
import java.util.concurrent.atomic.AtomicBoolean
import javax.microedition.khronos.egl.*
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.egl.EGLContext
import javax.microedition.khronos.egl.EGLDisplay
import javax.microedition.khronos.egl.EGLSurface
import javax.microedition.khronos.opengles.GL
import javax.microedition.khronos.opengles.GL10
import kotlin.math.max
import kotlin.math.min

class GLThread constructor(
	var onDrawCallback: (() -> Unit)? = null,
	var onResumeCallback: (() -> Unit)? = null,
	var onPauseCallback: (() -> Unit)? = null,
	var onFinishingCallback: (() -> Unit)? = null
) :
	Thread(TAG) {

	companion object {
		private const val TAG = "GLThread"
		private const val EGL_OPENGL_ES3_BIT = 0x00000040

		const val MAX_NUM_GRAPHICS_PRE_TASKS = 16

		private val defaultConfig = intArrayOf(
			EGL10.EGL_RENDERABLE_TYPE,
			EGL_OPENGL_ES3_BIT,
			EGL10.EGL_RED_SIZE, 8,
			EGL10.EGL_GREEN_SIZE, 8,
			EGL10.EGL_BLUE_SIZE, 8,
			EGL10.EGL_ALPHA_SIZE, 8,
			EGL10.EGL_DEPTH_SIZE, 16,
			EGL10.EGL_STENCIL_SIZE, 8,
			EGL10.EGL_NONE
		)
		private val msaaConfig = intArrayOf(
			EGL10.EGL_RENDERABLE_TYPE,
			EGL_OPENGL_ES3_BIT,
			EGL10.EGL_RED_SIZE, 8,
			EGL10.EGL_GREEN_SIZE, 8,
			EGL10.EGL_BLUE_SIZE, 8,
			EGL10.EGL_ALPHA_SIZE, 8,
			EGL10.EGL_DEPTH_SIZE, 16,
			EGL10.EGL_STENCIL_SIZE, 8,
			EGL10.EGL_SAMPLE_BUFFERS, 1,
			EGL10.EGL_SAMPLES, 4,
			EGL10.EGL_NONE
		)
	}

	val runNotifier = Object()
	var glRunList = ConcurrentLinkedQueue<() -> Unit>()
	val isDirty = AtomicBoolean(false)

	var renderer: GLSurfaceView.Renderer? = null
	var surface: SurfaceTexture? = null

	var useMSAA: Boolean = false
	var targetFrameRate = -1

	@Volatile
	private var finished = false
	private var isPaused = true
	private var egl: EGL10? = null
	private var eglDisplay: EGLDisplay? = null
	private var eglConfig: EGLConfig? = null
	private var eglContext: EGLContext? = null
	private var eglSurface: EGLSurface? = null
	private var gl: GL? = null
	private var width = 0
	private var height = 0

	@Volatile
	private var sizeChanged = true

	override fun run() {
		initGL()
		val gl10 = gl as GL10?
		val renderer = renderer ?: throw IllegalStateException("No renderer attached to GlTextureView")
		renderer.onSurfaceCreated(gl10, eglConfig)

		if (!isPaused) {
			onResumeCallback?.invoke()
		}

		while (!finished) {
			if ((!isDirty.get() && glRunList.isEmpty()) || isPaused) {
				var wasPaused = false
				do {
					if (isPaused) {
						if (!wasPaused) {
							onPauseCallback?.invoke()
						}
						wasPaused = true
					}
					try {
						synchronized(runNotifier) { runNotifier.wait(1000) }
					} catch (e: InterruptedException) {
						e.printStackTrace()
					}
				} while (isPaused && !finished)
				if (finished) {
					break
				} else if (wasPaused) {
					onResumeCallback?.invoke()
				}
			}
			isDirty.set(false)

			val timestampStartRender = System.nanoTime()
			checkCurrent()
			if (sizeChanged) {
				createSurface()
				renderer.onSurfaceChanged(gl10, width, height)
				sizeChanged = false
			}
			var i = 0
			while (glRunList.isNotEmpty() && i < MAX_NUM_GRAPHICS_PRE_TASKS) {
				glRunList.poll()?.invoke()
				i++
			}

			renderer.onDrawFrame(gl10)
			if (BuildConfig.DEBUG) {
				GLES32.glGetError().let {
					if (it != GLES32.GL_NO_ERROR) {
						Log.e(TAG, "OpenGL Render Error: $it")
					}
				}
			}

			if (surface != null) {
				if (egl?.eglSwapBuffers(eglDisplay, eglSurface) != true) {
					throw RuntimeException("Cannot swap buffers")
				}
			}

			onDrawCallback?.invoke()

			if (targetFrameRate > 0) {
				val renderTime = (System.nanoTime() - timestampStartRender) / 1000000
				try {
					val sleepTime = max(0, 1000 / min(1000, targetFrameRate) - renderTime)
					if (sleepTime > 0) sleep(sleepTime)
				} catch (e: InterruptedException) {
					// Ignore
				}
			}
		}
		onPauseCallback?.invoke()
		onFinishingCallback?.invoke()
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
		 */
		destroySurface()

		/*
		 * Create an EGL surface we can render into.
		 */
		if (surface != null) {
			eglSurface = try {
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
		} else {
			/*
			*   If no window surface is provided, create a matching one
			 */
			val surfAttr = intArrayOf(
					EGL10.EGL_WIDTH, width,
					EGL10.EGL_HEIGHT, height,
					EGL10.EGL_NONE
			)
			eglSurface = egl?.eglCreatePbufferSurface(eglDisplay, eglConfig, surfAttr)
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
		 */
		if (egl?.eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext) != true) {
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
		surface?.release()
		surface = null
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

		val glVersion = IntArray(2)
		GLES32.glGetIntegerv(GLES32.GL_MAJOR_VERSION, glVersion, 0)
		GLES32.glGetIntegerv(GLES32.GL_MINOR_VERSION, glVersion, 1)
		if (glVersion[0] != 3 || glVersion[1] != 2) {
			Log.e(TAG, "OpenGL ES 3.2. is not supported on this device! (${glVersion[0]}.${glVersion[1]})")
		}
	}

	fun createContext(egl: EGL10, eglDisplay: EGLDisplay?, eglConfig: EGLConfig?): EGLContext? {
		val attrib_list = intArrayOf(EGL14.EGL_CONTEXT_CLIENT_VERSION, 3, EGL10.EGL_NONE)
		return egl.eglCreateContext(eglDisplay, eglConfig, EGL10.EGL_NO_CONTEXT, attrib_list)
	}

	private fun chooseEglConfig(egl: EGL10): EGLConfig? {
		var config: EGLConfig? = null
		for (configSpec in getEglConfigs(useMSAA)) {
			val configsCount = IntArray(1)
			val configs = arrayOfNulls<EGLConfig>(1)
			egl.eglChooseConfig(eglDisplay, configSpec, configs, 1, configsCount)
			if (configsCount[0] > 0) {
				config = configs[0]
				break
			} else {
				Log.e(TAG, "iterative eglChooseConfig step failed " + GLUtils.getEGLErrorString(egl.eglGetError()))
			}
		}
		return config
	}

	private fun getEglConfigs(useMSAA: Boolean = false): Array<IntArray> {
		return if (useMSAA) arrayOf(msaaConfig, defaultConfig) else arrayOf(defaultConfig)
	}

	fun finish() {
		finished = true
		synchronized(runNotifier) { runNotifier.notify() }
	}

	fun doPause() {
		isPaused = true
	}

	fun doResume() {
		isPaused = false
		synchronized(runNotifier) { runNotifier.notify() }
	}

	fun queueEvent(clearQueueIfNotRunning: Boolean = false, r: () -> Unit) {
		if (clearQueueIfNotRunning && !isAlive) {
			glRunList.clear()
		} else {
			glRunList.add(r)
			requestRender()
		}
	}

	fun requestRender() {
		isDirty.set(true)
		synchronized(runNotifier) { runNotifier.notify() }
	}

	@Synchronized
	fun onWindowResize(w: Int, h: Int) {
		width = w
		height = h
		sizeChanged = true
	}

}
