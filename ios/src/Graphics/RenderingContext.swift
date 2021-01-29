import Foundation
import Metal
import MapCoreSharedModule

@objc
class RenderingContext: NSObject {
    weak var encoder: MTLRenderCommandEncoder?
    weak var sceneView: SceneView!
}

extension RenderingContext: MCRenderingContextInterface {
    func onSurfaceCreated() { }

    func setViewportSize(_ size: MCVec2I) {}

    func getViewportSize() -> MCVec2I { .init(x: 0, y: 0) }
}
