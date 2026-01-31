package io.openmobilemaps.mapscore.kmp.feature.map.interop

import io.openmobilemaps.mapscore.shared.graphics.CameraInterface as MapscoreCameraInterface
import io.openmobilemaps.mapscore.shared.graphics.ComputePassInterface as MapscoreComputePassInterface
import io.openmobilemaps.mapscore.shared.graphics.MaskingObjectInterface as MapscoreMaskingObjectInterface
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
}

actual open class ShaderFactoryInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
}

actual open class RenderingContextInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
}

actual open class SchedulerInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
}

actual open class TouchHandlerInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
}

actual open class PerformanceLoggerInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
}

actual open class CoordinateConversionHelperInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
}

actual open class RenderTargetInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
}

actual open class RenderPassInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
}

actual open class ComputePassInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
}

actual open class MaskingObjectInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
}

actual open class ErrorManager actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
}

actual open class CameraInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle
}

internal fun GraphicsObjectFactoryInterface.asMapscore(): MapscoreGraphicsObjectFactoryInterface? =
	nativeHandle as? MapscoreGraphicsObjectFactoryInterface

internal fun ShaderFactoryInterface.asMapscore(): MapscoreShaderFactoryInterface? =
	nativeHandle as? MapscoreShaderFactoryInterface

internal fun RenderingContextInterface.asMapscore(): MapscoreRenderingContextInterface? =
	nativeHandle as? MapscoreRenderingContextInterface

internal fun SchedulerInterface.asMapscore(): MapscoreSchedulerInterface? =
	nativeHandle as? MapscoreSchedulerInterface

internal fun TouchHandlerInterface.asMapscore(): MapscoreTouchHandlerInterface? =
	nativeHandle as? MapscoreTouchHandlerInterface

internal fun PerformanceLoggerInterface.asMapscore(): MapscorePerformanceLoggerInterface? =
	nativeHandle as? MapscorePerformanceLoggerInterface

internal fun CoordinateConversionHelperInterface.asMapscore(): MapscoreCoordinateConversionHelperInterface? =
	nativeHandle as? MapscoreCoordinateConversionHelperInterface

internal fun RenderTargetInterface.asMapscore(): MapscoreRenderTargetInterface? =
	nativeHandle as? MapscoreRenderTargetInterface

internal fun RenderPassInterface.asMapscore(): MapscoreRenderPassInterface? =
	nativeHandle as? MapscoreRenderPassInterface

internal fun ComputePassInterface.asMapscore(): MapscoreComputePassInterface? =
	nativeHandle as? MapscoreComputePassInterface

internal fun MaskingObjectInterface.asMapscore(): MapscoreMaskingObjectInterface? =
	nativeHandle as? MapscoreMaskingObjectInterface

internal fun ErrorManager.asMapscore(): MapscoreErrorManager? =
	nativeHandle as? MapscoreErrorManager

internal fun CameraInterface.asMapscore(): MapscoreCameraInterface? =
	nativeHandle as? MapscoreCameraInterface
