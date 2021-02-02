import Foundation
import MapCoreSharedModule
import MetalKit

public class SceneView: MTKView {
    private let scene: MCSceneInterface
    private let renderingContext: RenderingContext

    private var sizeChanged: Bool = false
    private var backgroundDisable = false

    private var framesToRender: UInt = 5
    private let framesToRenderAfterInvalidate: UInt = 5

    public init() {
        guard let scene = MCSceneInterface.create(GraphicsFactory(), shaderFactory: ShaderFactory()) else {
            fatalError("Can't create Scene")
        }
        self.scene = scene
        renderingContext = RenderingContext()
        super.init(frame: .zero, device: MetalContext.current.device)
        renderingContext.sceneView = self
        setup()
    }

    @available(*, unavailable)
    required init(coder _: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    private func setup() {
        device = MetalContext.current.device

        colorPixelFormat = MetalContext.current.colorPixelFormat
        framebufferOnly = false

        delegate = self

        depthStencilPixelFormat = .stencil8

        isMultipleTouchEnabled = true

        scene.setRenderingContext(renderingContext)
    }
}

extension SceneView: MCSceneCallbackInterface {
    public func invalidate() {
        isPaused = false
        framesToRender = framesToRenderAfterInvalidate
    }
}

extension SceneView: MTKViewDelegate {
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
            scene.getRenderingContext()?.setViewportSize(view.drawableSize.vec2)
            sizeChanged = false
        }

        scene.drawFrame()

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
