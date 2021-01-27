import Foundation
import MapCoreSharedModule

class Line2d: BaseGraphicsObject {
}

extension Line2d: MCLine2dInterface {
    func setLinePositions(_ positions: [MCVec2F]) {
    }

    func getAsGraphicsObject() -> MCGraphicsObjectInterface? {
        self
    }
}
