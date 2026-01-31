package io.openmobilemaps.mapscore.kmp.feature.map.interop

import MapCoreSharedModule.MCCameraInterfaceProtocol
import MapCoreSharedModule.MCComputePassInterfaceProtocol
import MapCoreSharedModule.MCErrorManager
import MapCoreSharedModule.MCGraphicsObjectFactoryInterfaceProtocol
import MapCoreSharedModule.MCMaskingObjectInterfaceProtocol
import MapCoreSharedModule.MCPerformanceLoggerInterfaceProtocol
import MapCoreSharedModule.MCRenderPassInterfaceProtocol
import MapCoreSharedModule.MCRenderTargetInterfaceProtocol
import MapCoreSharedModule.MCRenderingContextInterfaceProtocol
import MapCoreSharedModule.MCSchedulerInterfaceProtocol
import MapCoreSharedModule.MCShaderFactoryInterfaceProtocol
import MapCoreSharedModule.MCTouchHandlerInterfaceProtocol
import MapCoreSharedModule.MCCoordinateConversionHelperInterfaceProtocol

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

internal fun GraphicsObjectFactoryInterface.asMapCore(): MCGraphicsObjectFactoryInterfaceProtocol? =
	nativeHandle as? MCGraphicsObjectFactoryInterfaceProtocol

internal fun ShaderFactoryInterface.asMapCore(): MCShaderFactoryInterfaceProtocol? =
	nativeHandle as? MCShaderFactoryInterfaceProtocol

internal fun RenderingContextInterface.asMapCore(): MCRenderingContextInterfaceProtocol? =
	nativeHandle as? MCRenderingContextInterfaceProtocol

internal fun SchedulerInterface.asMapCore(): MCSchedulerInterfaceProtocol? =
	nativeHandle as? MCSchedulerInterfaceProtocol

internal fun TouchHandlerInterface.asMapCore(): MCTouchHandlerInterfaceProtocol? =
	nativeHandle as? MCTouchHandlerInterfaceProtocol

internal fun PerformanceLoggerInterface.asMapCore(): MCPerformanceLoggerInterfaceProtocol? =
	nativeHandle as? MCPerformanceLoggerInterfaceProtocol

internal fun CoordinateConversionHelperInterface.asMapCore(): MCCoordinateConversionHelperInterfaceProtocol? =
	nativeHandle as? MCCoordinateConversionHelperInterfaceProtocol

internal fun RenderTargetInterface.asMapCore(): MCRenderTargetInterfaceProtocol? =
	nativeHandle as? MCRenderTargetInterfaceProtocol

internal fun RenderPassInterface.asMapCore(): MCRenderPassInterfaceProtocol? =
	nativeHandle as? MCRenderPassInterfaceProtocol

internal fun ComputePassInterface.asMapCore(): MCComputePassInterfaceProtocol? =
	nativeHandle as? MCComputePassInterfaceProtocol

internal fun MaskingObjectInterface.asMapCore(): MCMaskingObjectInterfaceProtocol? =
	nativeHandle as? MCMaskingObjectInterfaceProtocol

internal fun ErrorManager.asMapCore(): MCErrorManager? =
	nativeHandle as? MCErrorManager

internal fun CameraInterface.asMapCore(): MCCameraInterfaceProtocol? =
	nativeHandle as? MCCameraInterfaceProtocol
