import Foundation
import MapCoreSharedModule
import Metal

class BaseShader: MCShaderProgramInterface {
    func getProgramName() -> String {
        ""
    }

    func setupProgram(_ context: MCRenderingContextInterface?) { }

    func preRender(_ context: MCRenderingContextInterface?) {
        guard let context = context as? RenderingContext,
              let encoder = context.encoder else { return }
        preRender(encoder: encoder, context: context)
    }

    func preRender(encoder: MTLRenderCommandEncoder,
                   context: RenderingContext) {

    }
}
