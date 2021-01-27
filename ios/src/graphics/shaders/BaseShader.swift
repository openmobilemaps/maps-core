import Foundation
import MapCoreSharedModule

class BaseShader: MCShaderProgramInterface {
    func getProgramName() -> String {
        ""
    }

    func setupProgram(_ context: MCRenderingContextInterface?) { }

    func preRender(_ context: MCRenderingContextInterface?) { }
}
