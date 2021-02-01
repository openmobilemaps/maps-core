import Foundation
import MapCoreSharedModule
import Metal

@objc
class RenderingContext: NSObject {
    weak var encoder: MTLRenderCommandEncoder?
    weak var sceneView: SceneView!
}

extension RenderingContext: MCRenderingContextInterface {
    func setupDrawFrame() {}

    func onSurfaceCreated() {}

    func setViewportSize(_: MCVec2I) {}

    func getViewportSize() -> MCVec2I { .init(x: 0, y: 0) }
}
