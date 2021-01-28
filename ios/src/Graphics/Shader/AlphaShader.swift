import Foundation
import Metal
import MapCoreSharedModule

class AlphaShader: BaseShader {
    private var alpha: Float = 1.0
    private var pipeline: MTLRenderPipelineState?
    private var stencilState: MTLDepthStencilState?

    override func setupProgram(_ context: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(PipelineKey.alphaShader)
        }

        if stencilState == nil {
            stencilState = MetalContext.current.device.makeDepthStencilState(descriptor: MTLDepthStencilDescriptor())
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let pipeline = pipeline,
              let stencilState = stencilState else { return }

        encoder.setRenderPipelineState(pipeline)
        encoder.setDepthStencilState(stencilState)

        encoder.setFragmentBytes(&alpha, length: MemoryLayout<Float>.stride, index: 1)
    }

}

extension AlphaShader: MCAlphaShaderInterface {
    func updateAlpha(_ value: Float) {
        alpha = value
    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}
