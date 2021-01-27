import Foundation
import MapCoreSharedModule

class Rectangle2d: BaseGraphicsObject {
    let shader: MCShaderProgramInterface
    init(shader: MCShaderProgramInterface) {
        self.shader = shader
    }
}


extension Rectangle2d: MCRectangle2dInterface {
    func setFrame(_ frame: MCRectF, textureCoordinates: MCRectF) { }

    func loadTexture(_ context: MCRenderingContextInterface?, textureHolder: MCTextureHolder?) { }

    func removeTextures(_ context: MCRenderingContextInterface?) { }

    func getAsGraphicsObject() -> MCGraphicsObjectInterface? { self }
}
