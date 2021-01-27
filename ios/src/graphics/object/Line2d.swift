import Foundation
import MapCoreSharedModule

class Line2d: BaseGraphicsObject {
    let shader: MCLineShaderProgramInterface
    init(shader: MCLineShaderProgramInterface) {
        self.shader = shader
    }
}

extension Line2d: MCLine2dInterface {
    func setLinePositions(_ positions: [MCVec2F]) {
    }

    func getAsGraphicsObject() -> MCGraphicsObjectInterface? {
        self
    }
}
