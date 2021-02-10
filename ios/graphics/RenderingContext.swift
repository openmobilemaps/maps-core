import Foundation
import MapCoreSharedModule
import Metal

@objc
class RenderingContext: NSObject {
    weak var encoder: MTLRenderCommandEncoder?
    weak var sceneView: MapView!

    var viewportSize: MCVec2I = .init(x: 0, y: 0)
}

extension RenderingContext: MCRenderingContextInterface {
    func setupDrawFrame() {}

    func onSurfaceCreated() {}

    func setViewportSize(_ newSize: MCVec2I) {
        viewportSize = newSize
    }

    func getViewportSize() -> MCVec2I { viewportSize }
}
