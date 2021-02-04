import UIKit
import MapCoreSharedModule

class MapViewTouchHandler: NSObject {

    private weak var touchHandler: MCTouchHandlerInterface!

    weak var mapView: UIView!

    private var activeTouches = Set<UITouch>()

    private var originalTouchLocations: [UITouch: CGPoint] = [:]

    init(touchHandler: MCTouchHandlerInterface?) {
        super.init()
        self.touchHandler = touchHandler
    }

    func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {

        touches.forEach {
            activeTouches.insert($0)
            originalTouchLocations[$0] = $0.location(in: mapView)
            touchHandler.onTouchEvent(activeTouches.asMCTouchEvent(in: mapView, scale: Float(mapView.contentScaleFactor), action: .DOWN))
        }
    }

    private func touchUp(_ touches: Set<UITouch>) {
        touches.forEach {
            activeTouches.insert($0)

            touchHandler.onTouchEvent(activeTouches.asMCTouchEvent(in: mapView, scale: Float(mapView.contentScaleFactor), action: .UP))

            activeTouches.remove($0)
            originalTouchLocations.removeValue(forKey: $0)
        }
    }

    func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        touchUp(touches)
    }

    func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        touchUp(touches)
    }

    func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        func CGPointDistanceSquared(from: CGPoint, to: CGPoint) -> CGFloat {
            return (from.x - to.x) * (from.x - to.x) + (from.y - to.y) * (from.y - to.y)
        }

        func CGPointDistance(from: CGPoint, to: CGPoint) -> CGFloat {
            return sqrt(CGPointDistanceSquared(from: from, to: to))
        }

        touches.forEach {
            activeTouches.insert($0)
            touchHandler.onTouchEvent(activeTouches.asMCTouchEvent(in: mapView, scale: Float(mapView.contentScaleFactor), action: .MOVE))
        }
    }

    func cancelAllTouches() {
        touchUp(activeTouches)
    }
}

private extension Set where Element == UITouch {
    func asMCTouchLocation(in view: UIView, scale: Float) -> [MCVec2F] {
        return map {
            let location = $0.location(in: view)
            let x = Float(location.x) * scale
            let y = Float(location.y) * scale
            return MCVec2F(x: x, y: y)
        }
    }

    func asMCTouchEvent(in view: UIView, scale: Float, action: MCTouchAction) -> MCTouchEvent {
        MCTouchEvent(pointers: asMCTouchLocation(in: view, scale: scale), touchAction: action)
    }
}
