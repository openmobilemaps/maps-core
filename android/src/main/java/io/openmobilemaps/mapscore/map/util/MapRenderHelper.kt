package io.openmobilemaps.mapscore.map.util

import android.graphics.Bitmap
import io.openmobilemaps.mapscore.extensions.isTerminal
import io.openmobilemaps.mapscore.shared.graphics.common.Vec2I
import io.openmobilemaps.mapscore.shared.map.LayerReadyState
import io.openmobilemaps.mapscore.shared.map.MapConfig
import io.openmobilemaps.mapscore.shared.map.MapReadyCallbackInterface
import io.openmobilemaps.mapscore.shared.map.coordinates.RectCoord
import io.openmobilemaps.mapscore.shared.map.scheduling.ExecutionEnvironment
import io.openmobilemaps.mapscore.shared.map.scheduling.TaskConfig
import io.openmobilemaps.mapscore.shared.map.scheduling.TaskInterface
import io.openmobilemaps.mapscore.shared.map.scheduling.TaskPriority
import kotlinx.coroutines.*
import java.util.concurrent.Semaphore

open class MapRenderHelper {

	companion object {
		@JvmStatic
		fun renderMap(
			coroutineScope: CoroutineScope,
			mapConfig: MapConfig,
			onSetupMap: (MapViewInterface) -> Unit,
			onStateUpdate: (MapViewRenderState) -> Unit,
			renderBounds: RectCoord,
			renderBoundsPaddingPc: Float = 0f,
			renderSizePx: Vec2I,
			renderDensity: Float = 72f,
			renderTimeoutSeconds: Float = 20f,
			destroyAfterRenderAction: Boolean = true,
			useMSAA: Boolean = false,
		) : OffscreenMapRenderer {
			onStateUpdate.invoke(MapViewRenderState.Loading)

			val mapRenderer = OffscreenMapRenderer(renderSizePx, renderDensity)
			mapRenderer.setupMap(coroutineScope, mapConfig, useMSAA)
			onSetupMap(mapRenderer)

			mapRenderer.requireMapInterface().getScheduler().addTask(object : TaskInterface() {
				override fun getConfig() = TaskConfig("render_task_start", 0, TaskPriority.NORMAL, ExecutionEnvironment.GRAPHICS)

				override fun run() {
					coroutineScope.launch(Dispatchers.Default) {
						render(mapRenderer, renderBounds, renderTimeoutSeconds, onStateUpdate, destroyAfterRenderAction, renderBoundsPaddingPc)
					}
				}
			})

			return mapRenderer
		}

		@JvmStatic
		fun reRenderMap(
			coroutineScope: CoroutineScope,
			mapRenderer: OffscreenMapRenderer,
			renderBounds: RectCoord,
			renderBoundsPaddingPc: Float = 0f,
			onStateUpdate: (MapViewRenderState) -> Unit,
			renderTimeoutSeconds: Float = 20f,
			destroyAfterRenderAction: Boolean = true,
		){
			mapRenderer.requireMapInterface().getScheduler().addTask(object : TaskInterface() {
				override fun getConfig() = TaskConfig("render_task_start", 0, TaskPriority.NORMAL, ExecutionEnvironment.GRAPHICS)

				override fun run() {
					coroutineScope.launch(Dispatchers.Default) {
						render(mapRenderer, renderBounds, renderTimeoutSeconds, onStateUpdate, destroyAfterRenderAction, renderBoundsPaddingPc)
					}
				}
			})
		}

		@JvmStatic
		protected suspend fun render(
			mapRenderer: OffscreenMapRenderer,
			renderBounds: RectCoord,
			timeoutSeconds: Float,
			onStateUpdate: (MapViewRenderState) -> Unit,
			destroyAfterRenderAction: Boolean,
			boundsPaddingPc: Float
		) {
			val coroutineContext = currentCoroutineContext()
			val drawSemaphore = Semaphore(0, true)
			mapRenderer.requireMapInterface().drawReadyFrame(renderBounds, boundsPaddingPc, timeoutSeconds, object : MapReadyCallbackInterface() {
				var prevState: LayerReadyState? = null
				override fun stateDidUpdate(state: LayerReadyState) {
					if (prevState.isTerminal()) return

					if (coroutineContext[Job]?.isActive == false) {
						if (destroyAfterRenderAction) {
							mapRenderer.destroy()
						}
						prevState = LayerReadyState.ERROR
						onStateUpdate(MapViewRenderState.Error)
						return
					}

					mapRenderer.setOnDrawCallback {
						drawSemaphore.release()
					}
					drawSemaphore.acquire()

					if (prevState == state || prevState.isTerminal()) return
					prevState = state
					when (state) {
						LayerReadyState.READY -> {
							val saveFrameSemaphore = Semaphore(0, true)
							mapRenderer.saveFrame(
								SaveFrameSpec(
									outputFormat = SaveFrameSpec.OutputFormat.BITMAP,
									pixelFormat = SaveFrameSpec.PixelFormat.ARGB_8888
								),
								object : SaveFrameCallback {
									override fun onResultBitmap(bitmap: Bitmap) {
										onStateUpdate(MapViewRenderState.Finished(bitmap))
										if (destroyAfterRenderAction) {
											mapRenderer.destroy()
										}
										saveFrameSemaphore.release()
									}

									override fun onError() {
										onStateUpdate(MapViewRenderState.Error)
										if (destroyAfterRenderAction) {
											mapRenderer.destroy()
										}
										saveFrameSemaphore.release()
									}

								})
							saveFrameSemaphore.acquire()
						}
						LayerReadyState.NOT_READY -> onStateUpdate(MapViewRenderState.Loading)
						LayerReadyState.ERROR -> {
							if (destroyAfterRenderAction) {
								mapRenderer.destroy()
							}
							onStateUpdate(MapViewRenderState.Error)
						}
						LayerReadyState.TIMEOUT_ERROR -> {
							if (destroyAfterRenderAction) {
								mapRenderer.destroy()
							}
							onStateUpdate(MapViewRenderState.Timeout)
						}
					}
				}
			})
		}
	}
}
