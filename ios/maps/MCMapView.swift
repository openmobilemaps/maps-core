import Foundation
import MapCoreSharedModule
import MetalKit
import os

open class MCMapView: MTKView {
    public let mapInterface: MCMapInterface
    private let renderingContext: RenderingContext

    private var sizeChanged: Bool = false
    private var backgroundDisable = false

    private var framesToRender: UInt = 1
    private let framesToRenderAfterInvalidate: UInt = 1

    private let touchHandler: MCMapViewTouchHandler

    public init(mapConfig: MCMapConfig) {
        let renderingContext = RenderingContext()
        guard let mapInterface = MCMapInterface.create(GraphicsFactory(),
                                                       shaderFactory: ShaderFactory(),
                                                       renderingContext: renderingContext,
                                                       mapConfig: mapConfig,
                                                       scheduler: MCScheduler(),
                                                       pixelDensity: Float(UIScreen.pixelsPerInch)) else {
            fatalError("Can't create MCMapInterface")
        }
        self.mapInterface = mapInterface
        self.renderingContext = renderingContext
        touchHandler = MCMapViewTouchHandler(touchHandler: mapInterface.getTouchHandler())
        super.init(frame: .zero, device: MetalContext.current.device)
        setup()
    }

    @available(*, unavailable)
    public required init(coder _: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    private func setup() {
        renderingContext.sceneView = self

        device = MetalContext.current.device

        colorPixelFormat = MetalContext.current.colorPixelFormat
        framebufferOnly = false

        delegate = self

        depthStencilPixelFormat = .stencil8

        isMultipleTouchEnabled = true

        mapInterface.setCallbackHandler(self)

        touchHandler.mapView = self

        mapInterface.resume()
    }

    /// The view’s background color.
    override public var backgroundColor: UIColor? {
        get {
            super.backgroundColor
        }
        set {
            super.backgroundColor = newValue
            mapInterface.setBackgroundColor(newValue?.mapCoreColor ?? UIColor.clear.mapCoreColor)
            isOpaque = newValue?.isOpaque ?? false
        }
    }
}

extension MCMapView: MCMapCallbackInterface {
    public func invalidate() {
        isPaused = false
        framesToRender = framesToRenderAfterInvalidate
    }
}

extension MCMapView: MTKViewDelegate {
    public func mtkView(_: MTKView, drawableSizeWillChange _: CGSize) {
        sizeChanged = true
        invalidate()
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
            mapInterface.setViewportSize(view.drawableSize.vec2)
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

public extension MCMapView {
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

public extension MCMapView {
    var camera: MCMapCamera2dInterface {
        mapInterface.getCamera()!
    }

    func add(layer: MCLayerInterface?) {
        mapInterface.addLayer(layer)
    }

    func insert(layer: MCLayerInterface?, at index: Int) {
        mapInterface.insertLayer(at: layer, at: Int32(index))
    }

    func insert(layer: MCLayerInterface?, above: MCLayerInterface?) {
        mapInterface.insertLayer(above: layer, above: above)
    }

    func insert(layer: MCLayerInterface?, below: MCLayerInterface?) {
        mapInterface.insertLayer(below: layer, below: below)
    }

    func remove(layer: MCLayerInterface?) {
        mapInterface.removeLayer(layer)
    }
}
