import Foundation
import MapCoreSharedModule

class AlphaShader: BaseShader {}

extension AlphaShader: MCAlphaShaderInterface {
    func updateAlpha(_ value: Float) {

    }

    func asShaderProgram(_ ptrToSelf: MCAlphaShaderInterface?) -> MCShaderProgramInterface? {
        self
    }
}
