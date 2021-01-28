import Foundation
import Metal
import MapCoreSharedModule

@objc
class RenderingContext: NSObject {
    var encoder: MTLRenderCommandEncoder?
}

extension RenderingContext: MCRenderingContextInterface {
    func onSurfaceCreated() { }

    func setViewportSize(_ size: MCVec2I) {}

    func getViewportSize() -> MCVec2I { .init(x: 0, y: 0) }
}
