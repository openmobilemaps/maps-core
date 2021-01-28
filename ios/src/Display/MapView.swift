//
//  UBMapView.swift
//  SwisstopoShared
//
//  Created by Joseph El Mallah on 06.02.20.
//  Copyright © 2020 Ubique Innovation AG. All rights reserved.
//
/*
import MetalKit

public protocol UBMapViewDelegate: AnyObject {
    func loadImage(baseURL: String, xCoord: Int, yCoord: Int, zoomLevel: Int, overlayName: String) -> Data?
    func loadVector(baseURL: String, xCoord: Int, yCoord: Int, zoomLevel: Int, overlayName: String) -> Data?
    func setRulerWidth(width: Float, widthInMetres: Float)
    func longPressed(coordinate: UBMMapCoordinate, radiusInMapUnits: Int32)
    func didTap(coordinate: UBMMapCoordinate)
}

public protocol UBMapViewInteractionDelegate: AnyObject {
    func didInteractWithMap()
    func didRotateMap(to angle: Double)
}

public class UBMapView: MTKView {
    private let renderer: UBMapViewRenderer
    private var activeTouches = Set<UITouch>()
    private var originalTouchLocations: [UITouch: CGPoint] = [:]

    public var mapDelegate: UBMapViewDelegate? {
        get { renderer.mapDelegate }
        set { renderer.mapDelegate = newValue }
    }

    public weak var mapInteractionDelegate: UBMapViewInteractionDelegate?

    public var sharedLibRenderer: UBMMapViewRenderer? {
        return renderer.sharedLibRenderer
    }

    public init(mapConfig: UBMMapConfig) {
        renderer = UBMapViewRenderer(mapConfig: mapConfig)

        super.init(frame: .zero, device: MetalContext.current.device)

        setup()
    }

    @available(*, unavailable)
    required init(coder _: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    private func setup() {
        device = MetalContext.current.device

        delegate = renderer
        colorPixelFormat = MetalContext.current.colorPixelFormat
        renderer.mapView = self
        framebufferOnly = false

        depthStencilPixelFormat = .stencil8

        isMultipleTouchEnabled = true
    }

    override public func didMoveToWindow() {
        super.didMoveToWindow()
        renderer.invalidate()
    }

    override public func didMoveToSuperview() {
        super.didMoveToSuperview()
        renderer.invalidate()
    }

    /// The view’s background color.
    override public var backgroundColor: UIColor? {
        get {
            return super.backgroundColor
        }
        set {
            super.backgroundColor = newValue
            clearColor = newValue?.metalClearColor ?? MTLClearColorMake(0, 0, 0, 0.0)
            isOpaque = newValue?.isOpaque ?? false
        }
    }

    // MARK: - Rotation

    func didRotate(to angle: Double) {
        mapInteractionDelegate?.didRotateMap(to: angle)
    }

    public func rotateToNorth() {
        renderer.sharedLibRenderer?.rotateToNorth()
    }

    public func currentMapImage() -> UIImage? {
        return currentDrawable?.texture.topo_toImage()
    }

    public func mapViewWillAppear() {
        renderer.willEnterForeground()
    }

    public func mapViewDidDisappear() {
        renderer.didEnterBackground()
    }
}

public extension UBMapView {
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        super.touchesBegan(touches, with: event)

        touches.forEach {
            activeTouches.insert($0)
            originalTouchLocations[$0] = $0.location(in: self)
            renderer.sharedLibRenderer?.onTouchEvent(activeTouches.asUBMTouchEvent(in: self, scale: Float(contentScaleFactor), action: .DOWN))
        }
    }

    private func touchUp(_ touches: Set<UITouch>) {
        touches.forEach {
            activeTouches.insert($0)

            renderer.sharedLibRenderer?.onTouchEvent(activeTouches.asUBMTouchEvent(in: self, scale: Float(contentScaleFactor), action: .UP))

            activeTouches.remove($0)
            originalTouchLocations.removeValue(forKey: $0)
        }
    }

    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        super.touchesEnded(touches, with: event)
        touchUp(touches)
    }

    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        super.touchesCancelled(touches, with: event)
        touchUp(touches)
    }

    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        func CGPointDistanceSquared(from: CGPoint, to: CGPoint) -> CGFloat {
            return (from.x - to.x) * (from.x - to.x) + (from.y - to.y) * (from.y - to.y)
        }

        func CGPointDistance(from: CGPoint, to: CGPoint) -> CGFloat {
            return sqrt(CGPointDistanceSquared(from: from, to: to))
        }

        super.touchesMoved(touches, with: event)
        touches.forEach {
            activeTouches.insert($0)

            if let originalLocation = originalTouchLocations[$0],
               CGPointDistance(from: originalLocation, to: $0.location(in: self)) > 10.0 {
                mapInteractionDelegate?.didInteractWithMap()
            }

            renderer.sharedLibRenderer?.onTouchEvent(activeTouches.asUBMTouchEvent(in: self, scale: Float(contentScaleFactor), action: .MOVE))
        }
    }

    func cancelAllTouches() {
        touchUp(activeTouches)
    }

    func invalidate() {
        renderer.invalidate()
    }

    func invalidateScale() {
        renderer.sharedLibRenderer?.invalidateScale()
    }
}

private extension Set where Element == UITouch {
    func asUBMTouchLocation(in view: UIView, scale: Float) -> [UBMTouchLocation] {
        return map {
            let location = $0.location(in: view)
            let x = Float(location.x) * scale
            let y = Float(location.y) * scale
            return UBMTouchLocation(x: x, y: y)
        }
    }

    func asUBMTouchEvent(in view: UIView, scale: Float, action: UBMTouchAction) -> UBMTouchEvent {
        UBMTouchEvent(pointer: asUBMTouchLocation(in: view, scale: scale), action: action)
    }
}

private extension MTLTexture {
    func topo_toImage() -> UIImage? {
        let context = CIContext()
        let cImg = CIImage(mtlTexture: self, options: nil)!
        return context.createCGImage(cImg, from: cImg.extent)?.topo_toImage()
    }
}

private extension CGImage {
    func topo_toImage() -> UIImage? {
        UIGraphicsBeginImageContext(CGSize(width: width, height: height))
        let context = UIGraphicsGetCurrentContext()
        context?.draw(self, in: CGRect(x: 0, y: 0, width: width, height: height))

        let newImage = UIGraphicsGetImageFromCurrentImageContext()
        UIGraphicsEndImageContext()

        return newImage
    }
}
*/
