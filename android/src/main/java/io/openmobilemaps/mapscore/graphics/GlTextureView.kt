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

import android.content.Context
import android.graphics.SurfaceTexture
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import android.view.TextureView
import android.view.TextureView.SurfaceTextureListener
import io.openmobilemaps.mapscore.shared.map.PerformanceLoggerInterface


open class GlTextureView @JvmOverloads constructor(context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0) :
	TextureView(
		context, attrs, defStyleAttr
	), SurfaceTextureListener {

	init {
		surfaceTextureListener = this
	}

	private var renderer: GLSurfaceView.Renderer? = null
	private var glThread: GLThread? = null

	private var useMSAA = false

	private val pendingTaskQueue = ArrayDeque<Pair<Boolean, () -> Unit>>()
	private var pendingTargetFrameRate = -1
	private var pendingEnforcedFinishInterval: Int? = null
	private var shouldResume = false
	private var performanceLoggers: List<PerformanceLoggerInterface> = emptyList()

	override fun onSurfaceTextureAvailable(surfaceTexture: SurfaceTexture, width: Int, height: Int) {
		val currentGlThread = glThread
		if (currentGlThread != null) {
			currentGlThread.apply {
				surface = surfaceTexture
				onWindowResize(width, height)
				if (shouldResume) {
					doResume()
				}
			}
		} else {
			glThread = GLThread(
				onResumeCallback = this::onGlThreadResume,
				onPauseCallback = this::onGlThreadPause,
				onFinishingCallback = this::onGlThreadFinishing
			).apply {
				surface = surfaceTexture
				onWindowResize(getWidth(), getHeight())
				useMSAA = this@GlTextureView.useMSAA
				renderer = this@GlTextureView.renderer
				targetFrameRate = pendingTargetFrameRate
				enforcedFinishInterval = pendingEnforcedFinishInterval
				performanceLoggers = this@GlTextureView.performanceLoggers
				if (shouldResume) {
					doResume()
				}
				start()
			}
		}
		while (pendingTaskQueue.isNotEmpty()) {
			pendingTaskQueue.removeFirstOrNull()?.let { queueEvent(it.first, it.second) }
		}
	}

	override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {
		glThread?.onWindowResize(width, height)
	}

	override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
		pauseGlThread()
		return false
	}

	protected open fun onGlThreadResume() {}

	protected open fun onGlThreadPause() {}

	protected open fun onGlThreadFinishing() {}

	override fun onSurfaceTextureUpdated(surface: SurfaceTexture) {}

	fun setRenderer(renderer: GLSurfaceView.Renderer?) {
		this.renderer = renderer
	}

	fun configureGL(useMSAA: Boolean) {
		this.useMSAA = useMSAA
	}

	fun requestRender() {
		glThread?.requestRender()
	}

	fun queueEvent(clearQueueIfNotRunning: Boolean = false, r: () -> Unit) {
		glThread?.queueEvent(clearQueueIfNotRunning, r) ?: pendingTaskQueue.add(Pair(clearQueueIfNotRunning, r))
	}

	fun setTargetFrameRate(frameRate: Int) {
		pendingTargetFrameRate = frameRate
		glThread?.targetFrameRate = frameRate
	}

	fun setEnforcedFinishInterval(enforcedFinishInterval: Int?) {
		pendingEnforcedFinishInterval = enforcedFinishInterval
		glThread?.enforcedFinishInterval = enforcedFinishInterval
	}

	fun pauseGlThread() {
		shouldResume = false
		glThread?.doPause()
	}

	fun resumeGlThread() {
		glThread?.doResume()
		shouldResume = true
	}

	fun finishGlThread() {
		val glThread = glThread
		this.glThread = null
		glThread?.finish()
	}

	open fun setPerformanceLoggers(performanceLoggers: List<PerformanceLoggerInterface>) {
		this.performanceLoggers = performanceLoggers
		glThread?.performanceLoggers = performanceLoggers
	}
}
