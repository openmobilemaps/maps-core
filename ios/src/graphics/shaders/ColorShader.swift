import Foundation
import MapCoreSharedModule

class ColorShader: BaseShader {}

extension ColorShader: MCColorShaderInterface {
    func setColor(_ red: Float, green: Float, blue: Float, alpha: Float) {

    }

    func asShaderProgram(_ ptrToSelf: MCColorShaderInterface?) -> MCShaderProgramInterface? {
        self
    }
}
