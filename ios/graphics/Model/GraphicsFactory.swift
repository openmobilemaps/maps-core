import Foundation
import MapCoreSharedModule

class GraphicsFactory: MCGraphicsObjectFactoryInterface {
    func createRectangle(_ shader: MCShaderProgramInterface?) -> MCRectangle2dInterface? {
        guard let shader = shader else { fatalError("No Shader provided") }
        return Rectangle2d(shader: shader, metalContext: .current)
    }

    func createLine(_ lineShader: MCLineShaderProgramInterface?) -> MCLine2dInterface? {
        guard let shader = lineShader else { fatalError("No Shader provided") }
        return Line2d(shader: shader, metalContext: .current)
    }

    func createPolygon(_ shader: MCShaderProgramInterface?) -> MCPolygon2dInterface? {
        guard let shader = shader else { fatalError("No Shader provided") }
        return Polygon2d(shader: shader, metalContext: .current)
    }
}
