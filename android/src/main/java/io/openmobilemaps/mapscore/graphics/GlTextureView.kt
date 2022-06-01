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


open class GlTextureView @JvmOverloads constructor(context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0) :
	TextureView(
		context, attrs, defStyleAttr
	), SurfaceTextureListener {

	init {
		surfaceTextureListener = this
	}

	private var useMSAA = false
	private var renderer: GLSurfaceView.Renderer? = null
	private var glThread: GLThread? = null

	private val pendingTaskQueue = ArrayDeque<Pair<Boolean, () -> Unit>>()
	private var pendingTargetFrameRate = -1

	override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
		glThread = GLThread(surface, useMSAA).apply {
			onWindowResize(getWidth(), getHeight())
			renderer = this@GlTextureView.renderer
			targetFrameRate = pendingTargetFrameRate
			while (pendingTaskQueue.isNotEmpty()) {
				pendingTaskQueue.removeFirstOrNull()?.let { queueEvent(it.first, it.second) }
			}
			start()
		}
	}

	override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {
		glThread?.onWindowResize(width, height)
	}

	override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
		glThread?.finish()
		return false
	}

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
}