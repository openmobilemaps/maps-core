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
import MapCoreSharedModule.MCCoordinateConversionHelperInterface

actual open class GraphicsObjectFactoryInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapCore(): MCGraphicsObjectFactoryInterfaceProtocol? =
		nativeHandle as? MCGraphicsObjectFactoryInterfaceProtocol
}

actual open class ShaderFactoryInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapCore(): MCShaderFactoryInterfaceProtocol? =
		nativeHandle as? MCShaderFactoryInterfaceProtocol
}

actual open class RenderingContextInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapCore(): MCRenderingContextInterfaceProtocol? =
		nativeHandle as? MCRenderingContextInterfaceProtocol
}

actual open class SchedulerInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapCore(): MCSchedulerInterfaceProtocol? =
		nativeHandle as? MCSchedulerInterfaceProtocol
}

actual open class TouchHandlerInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapCore(): MCTouchHandlerInterfaceProtocol? =
		nativeHandle as? MCTouchHandlerInterfaceProtocol
}

actual open class PerformanceLoggerInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapCore(): MCPerformanceLoggerInterfaceProtocol? =
		nativeHandle as? MCPerformanceLoggerInterfaceProtocol
}

actual open class CoordinateConversionHelperInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapCore(): MCCoordinateConversionHelperInterface? =
		nativeHandle as? MCCoordinateConversionHelperInterface
}

actual open class RenderTargetInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapCore(): MCRenderTargetInterfaceProtocol? =
		nativeHandle as? MCRenderTargetInterfaceProtocol
}

actual open class RenderPassInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapCore(): MCRenderPassInterfaceProtocol? =
		nativeHandle as? MCRenderPassInterfaceProtocol
}

actual open class ComputePassInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapCore(): MCComputePassInterfaceProtocol? =
		nativeHandle as? MCComputePassInterfaceProtocol
}

actual open class MaskingObjectInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapCore(): MCMaskingObjectInterfaceProtocol? =
		nativeHandle as? MCMaskingObjectInterfaceProtocol
}

actual open class ErrorManager actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapCore(): MCErrorManager? =
		nativeHandle as? MCErrorManager
}

actual open class CameraInterface actual constructor(nativeHandle: Any?) {
	protected actual val nativeHandle: Any? = nativeHandle

	internal fun asMapCore(): MCCameraInterfaceProtocol? =
		nativeHandle as? MCCameraInterfaceProtocol
}
