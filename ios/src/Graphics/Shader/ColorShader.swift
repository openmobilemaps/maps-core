import Foundation
import MapCoreSharedModule
import Metal

class ColorShader: BaseShader {
    private var color: SIMD4<Float> = SIMD4<Float>([0.0, 0.0, 0.0, 0.0])

    private var pipeline: MTLRenderPipelineState?

    private var stencilState: MTLDepthStencilState?

    override func setupProgram(_ context: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(PipelineKey.colorShader)
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let pipeline = pipeline else { return }
        encoder.setRenderPipelineState(pipeline)
        encoder.setFragmentBytes(&color, length: MemoryLayout<SIMD4<Float>>.stride, index: 1)
    }
}

extension ColorShader: MCColorShaderInterface {
    func setColor(_ red: Float, green: Float, blue: Float, alpha: Float) {
        color = [red, green, blue, alpha]
    }

    func asShaderProgram(_ ptrToSelf: MCColorShaderInterface?) -> MCShaderProgramInterface? {
        self
    }
}
