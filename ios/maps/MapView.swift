import Foundation
import MapCoreSharedModule
import MetalKit

open class MapView: MTKView {
    public let mapInterface: MCMapInterface
    private let renderingContext: RenderingContext

    private var sizeChanged: Bool = false
    private var backgroundDisable = false

    private var framesToRender: UInt = 1
    private let framesToRenderAfterInvalidate: UInt = 1

    private let touchHandler: MapViewTouchHandler

    public init(mapConfig: MCMapConfig) {
        let renderingContext = RenderingContext()
        guard let mapInterface = MCMapInterface.create(GraphicsFactory(),
                                                       shaderFactory: ShaderFactory(),
                                                       renderingContext: renderingContext,
                                                       mapConfig: mapConfig,
                                                       scheduler: Scheduler()) else {
            fatalError("Can't create MCMapInterface")
        }
        self.mapInterface = mapInterface
        self.renderingContext = renderingContext
        mapInterface.addDefaultTouchHandler(Float(UIScreen.pixelsPerInch))
        touchHandler = .init(touchHandler: mapInterface.getTouchHandler())
        super.init(frame: .zero, device: MetalContext.current.device)
        renderingContext.sceneView = self
        setup()
    }

    @available(*, unavailable)
    public required init(coder _: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    private func setup() {
        device = MetalContext.current.device

        colorPixelFormat = MetalContext.current.colorPixelFormat
        framebufferOnly = false

        delegate = self

        depthStencilPixelFormat = .stencil8

        isMultipleTouchEnabled = true

        mapInterface.setCallbackHandler(self)

        touchHandler.mapView = self

        if let camera = MCMapCamera2dInterface.create(mapInterface, screenDensityPpi: Float(UIScreen.pixelsPerInch)) {
            mapInterface.setCamera(camera.asCameraInterface())
        }

        mapInterface.resume()
    }
}

extension MapView: MCMapCallbackInterface {
    public func invalidate() {
        isPaused = false
        framesToRender = framesToRenderAfterInvalidate
    }
}

extension MapView: MTKViewDelegate {
    public func mtkView(_: MTKView, drawableSizeWillChange _: CGSize) {
        sizeChanged = true
        isPaused = false
    }

    public func draw(in view: MTKView) {
        guard !backgroundDisable else {
            return // don't execute metal calls in background
        }

        guard framesToRender != 0 else {
            isPaused = true
            return
        }

        framesToRender -= 1

        guard let renderPassDescriptor = view.currentRenderPassDescriptor,
              let commandBuffer = MetalContext.current.commandQueue.makeCommandBuffer(),
              let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor)
        else {
            return
        }

        renderingContext.encoder = renderEncoder

        // Shared lib stuff
        if sizeChanged {
            renderingContext.setViewportSize(view.drawableSize.vec2)
            sizeChanged = false
        }

        mapInterface.drawFrame()

        renderEncoder.endEncoding()

        guard let drawable = view.currentDrawable else {
            return
        }

        commandBuffer.present(drawable)
        commandBuffer.commit()
    }
}

extension CGSize {
    var vec2: MCVec2I {
        MCVec2I(x: Int32(width),
                y: Int32(height))
    }
}

public extension MapView {
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        super.touchesBegan(touches, with: event)
        touchHandler.touchesBegan(touches, with: event)
    }

    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        super.touchesEnded(touches, with: event)
        touchHandler.touchesEnded(touches, with: event)
    }

    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        super.touchesCancelled(touches, with: event)
        touchHandler.touchesCancelled(touches, with: event)
    }

    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        super.touchesMoved(touches, with: event)
        touchHandler.touchesMoved(touches, with: event)
    }
}
