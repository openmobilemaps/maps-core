import Foundation
import MapCoreSharedModule

class Polygon2d: BaseGraphicsObject {
    let shader: MCShaderProgramInterface
    init(shader: MCShaderProgramInterface) {
        self.shader = shader
    }
}

extension Polygon2d: MCPolygon2dInterface {
    func setPolygonPositions(_ positions: [MCVec2F], holes: [[MCVec2F]], isConvex: Bool) {
    }

    func getAsGraphicsObject() -> MCGraphicsObjectInterface? {
        self
    }
}
