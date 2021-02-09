import Foundation
import MapCoreSharedModule
import Metal
import UIKit

class Rectangle2d: BaseGraphicsObject {
    private var verticesBuffer: MTLBuffer?

    private var indicesBuffer: MTLBuffer?

    private var indicesCount: Int = 0

    private var texture: MTLTexture?

    private var shader: MCShaderProgramInterface

    init(shader: MCShaderProgramInterface, metalContext: MetalContext) {
        self.shader = shader
        super.init(device: metalContext.device,
                   sampler: metalContext.samplerLibrary.value(.magLinear))

        let image = UIImage(named: "ubique")
        loadTexture(try! TextureHolder(image!.cgImage!))
    }

    override func render(encoder: MTLRenderCommandEncoder,
                         context: RenderingContext,
                         renderPass _: MCRenderPassConfig,
                         mvpMatrix: Int64)
    {
        guard let verticesBuffer = verticesBuffer,
              let indicesBuffer = indicesBuffer else { return }
        shader.setupProgram(context)
        shader.preRender(context)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)
        let matrixPointer = UnsafeRawPointer(bitPattern: Int(mvpMatrix))!
        encoder.setVertexBytes(matrixPointer, length: 64, index: 1)

        encoder.setFragmentSamplerState(sampler, index: 0)

        if let texture = texture {
            encoder.setFragmentTexture(texture, index: 0)
        }

        encoder.drawIndexedPrimitives(type: .triangle,
                                      indexCount: indicesCount,
                                      indexType: .uint16,
                                      indexBuffer: indicesBuffer,
                                      indexBufferOffset: 0)
    }
}

extension Rectangle2d: MCRectangle2dInterface {
    func setFrame(_ frame: MCRectD, textureCoordinates: MCRectD) {
        /*
         The quad is made out of 4 vertices as following
         B----C
         |    |
         |    |
         A----D
         Where A-C are joined to form two triangles
         */
        let minX = frame.xF
        let minY = frame.yF
        let maxX = minX + frame.widthF
        let maxY = minY + frame.heightF
        let vertecies: [Vertex] = [
            Vertex(x: minX, y: maxY, textureU: textureCoordinates.xF, textureV: textureCoordinates.heightF), // A
            Vertex(x: minX, y: minY, textureU: textureCoordinates.xF, textureV: textureCoordinates.yF), // B
            Vertex(x: maxX, y: minY, textureU: textureCoordinates.widthF, textureV: textureCoordinates.yF), // C
            Vertex(x: maxX, y: maxY, textureU: textureCoordinates.widthF, textureV: textureCoordinates.heightF), // D
        ]
        let indices: [UInt16] = [
            0, 1, 2, // ABC
            0, 2, 3, // ACD
        ]

        guard let verticesBuffer = device.makeBuffer(bytes: vertecies, length: MemoryLayout<Vertex>.stride * vertecies.count, options: []), let indicesBuffer = device.makeBuffer(bytes: indices, length: MemoryLayout<UInt16>.stride * indices.count, options: []) else {
            fatalError("Cannot allocate buffers")
        }

        indicesCount = indices.count
        self.verticesBuffer = verticesBuffer
        self.indicesBuffer = indicesBuffer
    }

    func loadTexture(_ textureHolder: MCTextureHolderInterface?) {
        guard let textureHolder = textureHolder as? TextureHolder else {
            fatalError("unexpected TextureHolder")
        }
        texture = textureHolder.texture
    }

    func removeTexture() {
        texture = nil
    }

    func asGraphicsObject() -> MCGraphicsObjectInterface? { self }
}
