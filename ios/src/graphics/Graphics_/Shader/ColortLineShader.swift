import Foundation
import MapCoreSharedModule
import Metal

class ColorLineShader: BaseShader {
    private var color = SIMD4<Float>([0.0, 0.0, 0.0, 0.0])

    private var miter: Float = 0.0

    private var linePipeline: MTLRenderPipelineState?

    private var pointPipeline: MTLRenderPipelineState?
}

extension ColorLineShader: MCLineShaderProgramInterface {
    func getRectProgramName() -> String { "" }

    func getPointProgramName() -> String { "" }

    func setupRectProgram(_: MCRenderingContextInterface?) {
        if linePipeline == nil {
            linePipeline = MetalContext.current.pipelineLibrary.value(PipelineKey.lineShader)
        }
    }

    func setupPointProgram(_: MCRenderingContextInterface?) {
        if pointPipeline == nil {
            pointPipeline = MetalContext.current.pipelineLibrary.value(PipelineKey.pointShader)
        }
    }

    func preRenderRect(_ context: MCRenderingContextInterface?) {
        guard let context = context as? RenderingContext,
              let encoder = context.encoder,
              let pipeline = linePipeline else { return }

        encoder.setRenderPipelineState(pipeline)

        var m = miter
        encoder.setVertexBytes(&m, length: MemoryLayout<Float>.stride, index: 2)

        var c = SIMD4<Float>(color)
        encoder.setFragmentBytes(&c, length: MemoryLayout<SIMD4<Float>>.stride, index: 1)
    }

    func preRenderPoint(_ context: MCRenderingContextInterface?) {
        guard let context = context as? RenderingContext,
              let encoder = context.encoder,
              let pipeline = pointPipeline else { return }

        encoder.setRenderPipelineState(pipeline)

        // radius is twice the miter
        var m = 2.0 * miter
        encoder.setVertexBytes(&m, length: MemoryLayout<Float>.stride, index: 2)

        var c = SIMD4<Float>(color)
        encoder.setFragmentBytes(&c, length: MemoryLayout<SIMD4<Float>>.stride, index: 1)
    }
}

extension ColorLineShader: MCColorLineShaderInterface {
    func setColor(_ red: Float, green: Float, blue: Float, alpha: Float) {
        color = [red, green, blue, alpha]
    }

    func setMiter(_ miter: Float) {
        self.miter = miter
    }

    func asLineShaderProgram() -> MCLineShaderProgramInterface? {
        self
    }
}
