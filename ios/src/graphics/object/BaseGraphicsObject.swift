import Foundation
import MapCoreSharedModule

class BaseGraphicsObject {
    var isReadyFlag: Bool = false
    var context: MCRenderingContextInterface!
}

extension BaseGraphicsObject: MCGraphicsObjectInterface {


    func setup(_ context: MCRenderingContextInterface?) {
        self.context = context
    }

    func clear() { }

    func isReady() -> Bool { isReadyFlag }

    func render(_ context: MCRenderingContextInterface?, renderPass: MCRenderPassConfig, mvpMatrix: Int64) {
    }
}
