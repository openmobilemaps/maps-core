import Foundation
import MapCoreSharedModule
import Metal

class BaseGraphicsObject {
    private var ReadyFlag_: Bool = false

    private var context: MCRenderingContextInterface!

    let device: MTLDevice

    let sampler: MTLSamplerState

    init(device: MTLDevice, sampler: MTLSamplerState) {
        self.device = device
        self.sampler = sampler
    }

    func render(encoder _: MTLRenderCommandEncoder,
                context _: RenderingContext,
                renderPass _: MCRenderPassConfig,
                mvpMatrix _: Int64)
    {
        fatalError("has to be overwritten by subclass")
    }
}

extension BaseGraphicsObject: MCGraphicsObjectInterface {
    func setup(_ context: MCRenderingContextInterface?) {
        self.context = context
    }

    func clear() {}

    func isReady() -> Bool { ReadyFlag_ }

    func render(_ context: MCRenderingContextInterface?, renderPass: MCRenderPassConfig, mvpMatrix: Int64) {
        guard let context = context as? RenderingContext,
              let encoder = context.encoder
        else { return }
        render(encoder: encoder,
               context: context,
               renderPass: renderPass,
               mvpMatrix: mvpMatrix)
    }
}
