package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.shared.graphics.CameraInterface as MapscoreCameraInterface
import io.openmobilemaps.mapscore.shared.graphics.ComputePassInterface as MapscoreComputePassInterface
import io.openmobilemaps.mapscore.shared.graphics.objects.MaskingObjectInterface as MapscoreMaskingObjectInterface
import io.openmobilemaps.mapscore.shared.graphics.RenderPassInterface as MapscoreRenderPassInterface
import io.openmobilemaps.mapscore.shared.graphics.RenderTargetInterface as MapscoreRenderTargetInterface
import io.openmobilemaps.mapscore.shared.graphics.RenderingContextInterface as MapscoreRenderingContextInterface
import io.openmobilemaps.mapscore.shared.graphics.objects.GraphicsObjectFactoryInterface as MapscoreGraphicsObjectFactoryInterface
import io.openmobilemaps.mapscore.shared.graphics.shader.ShaderFactoryInterface as MapscoreShaderFactoryInterface
import io.openmobilemaps.mapscore.shared.map.ErrorManager as MapscoreErrorManager
import io.openmobilemaps.mapscore.shared.map.PerformanceLoggerInterface as MapscorePerformanceLoggerInterface
import io.openmobilemaps.mapscore.shared.map.controls.TouchHandlerInterface as MapscoreTouchHandlerInterface
import io.openmobilemaps.mapscore.shared.map.coordinates.CoordinateConversionHelperInterface as MapscoreCoordinateConversionHelperInterface
import io.openmobilemaps.mapscore.shared.map.scheduling.SchedulerInterface as MapscoreSchedulerInterface

actual open class GraphicsObjectFactoryInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapscore(): MapscoreGraphicsObjectFactoryInterface? =
		nativeHandle as? MapscoreGraphicsObjectFactoryInterface
}

actual open class ShaderFactoryInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapscore(): MapscoreShaderFactoryInterface? =
		nativeHandle as? MapscoreShaderFactoryInterface
}

actual open class RenderingContextInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapscore(): MapscoreRenderingContextInterface? =
		nativeHandle as? MapscoreRenderingContextInterface
}

actual open class SchedulerInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapscore(): MapscoreSchedulerInterface? =
		nativeHandle as? MapscoreSchedulerInterface
}

actual open class TouchHandlerInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapscore(): MapscoreTouchHandlerInterface? =
		nativeHandle as? MapscoreTouchHandlerInterface
}

actual open class PerformanceLoggerInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapscore(): MapscorePerformanceLoggerInterface? =
		nativeHandle as? MapscorePerformanceLoggerInterface
}

actual open class CoordinateConversionHelperInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapscore(): MapscoreCoordinateConversionHelperInterface? =
		nativeHandle as? MapscoreCoordinateConversionHelperInterface
}

actual open class RenderTargetInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapscore(): MapscoreRenderTargetInterface? =
		nativeHandle as? MapscoreRenderTargetInterface
}

actual open class RenderPassInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapscore(): MapscoreRenderPassInterface? =
		nativeHandle as? MapscoreRenderPassInterface
}

actual open class ComputePassInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapscore(): MapscoreComputePassInterface? =
		nativeHandle as? MapscoreComputePassInterface
}

actual open class MaskingObjectInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapscore(): MapscoreMaskingObjectInterface? =
		nativeHandle as? MapscoreMaskingObjectInterface
}

actual open class ErrorManager actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapscore(): MapscoreErrorManager? =
		nativeHandle as? MapscoreErrorManager
}

actual open class CameraInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapscore(): MapscoreCameraInterface? =
		nativeHandle as? MapscoreCameraInterface
}
