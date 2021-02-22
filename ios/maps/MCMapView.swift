/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import Foundation
import MapCoreSharedModule
import MetalKit
import os

enum MCMapViewError: Error {
    case invalidLayerType
}

open class MCMapView: MTKView {
    public let mapInterface: MCMapInterface
    private let renderingContext: RenderingContext

    private var sizeChanged: Bool = false
    private var backgroundDisable = false

    private var framesToRender: UInt = 1
    private let framesToRenderAfterInvalidate: UInt = 1

    private let touchHandler: MCMapViewTouchHandler

    public init(mapConfig: MCMapConfig) {
        let renderingContext = RenderingContext()
        guard let mapInterface = MCMapInterface.create(GraphicsFactory(),
                                                       shaderFactory: ShaderFactory(),
                                                       renderingContext: renderingContext,
                                                       mapConfig: mapConfig,
                                                       scheduler: MCScheduler(),
                                                       pixelDensity: Float(UIScreen.pixelsPerInch)) else {
            fatalError("Can't create MCMapInterface")
        }
        self.mapInterface = mapInterface
        self.renderingContext = renderingContext
        touchHandler = MCMapViewTouchHandler(touchHandler: mapInterface.getTouchHandler())
        super.init(frame: .zero, device: MetalContext.current.device)
        setup()
    }

    @available(*, unavailable)
    public required init(coder _: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    private func setup() {
        renderingContext.sceneView = self

        device = MetalContext.current.device

        colorPixelFormat = MetalContext.current.colorPixelFormat
        framebufferOnly = false

        delegate = self

        depthStencilPixelFormat = .stencil8

        isMultipleTouchEnabled = true

        mapInterface.setCallbackHandler(self)

        touchHandler.mapView = self

        mapInterface.resume()
    }

    /// The view’s background color.
    override public var backgroundColor: UIColor? {
        get {
            super.backgroundColor
        }
        set {
            super.backgroundColor = newValue
            mapInterface.setBackgroundColor(newValue?.mapCoreColor ?? UIColor.clear.mapCoreColor)
            isOpaque = newValue?.isOpaque ?? false
        }
    }
}

extension MCMapView: MCMapCallbackInterface {
    public func invalidate() {
        isPaused = false
        framesToRender = framesToRenderAfterInvalidate
    }
}

extension MCMapView: MTKViewDelegate {
    public func mtkView(_: MTKView, drawableSizeWillChange _: CGSize) {
        sizeChanged = true
        invalidate()
    }

    public func draw(in view: MTKView) {
        guard !backgroundDisable else {
            return // don't execute metal calls in background
        }

        guard framesToRender != 0 else {
            isPaused = true
            return
        }

        framesToRender -= 1

        guard let renderPassDescriptor = view.currentRenderPassDescriptor,
              let commandBuffer = MetalContext.current.commandQueue.makeCommandBuffer(),
              let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor)
        else {
            return
        }

        renderingContext.encoder = renderEncoder

        // Shared lib stuff
        if sizeChanged {
            mapInterface.setViewportSize(view.drawableSize.vec2)
            sizeChanged = false
        }
        
        mapInterface.drawFrame()

        renderEncoder.endEncoding()

        guard let drawable = view.currentDrawable else {
            return
        }

        commandBuffer.present(drawable)
        commandBuffer.commit()
    }
}

extension CGSize {
    var vec2: MCVec2I {
        MCVec2I(x: Int32(width),
                y: Int32(height))
    }
}

public extension MCMapView {
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        super.touchesBegan(touches, with: event)
        touchHandler.touchesBegan(touches, with: event)
    }

    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        super.touchesEnded(touches, with: event)
        touchHandler.touchesEnded(touches, with: event)
    }

    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        super.touchesCancelled(touches, with: event)
        touchHandler.touchesCancelled(touches, with: event)
    }

    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        super.touchesMoved(touches, with: event)
        touchHandler.touchesMoved(touches, with: event)
    }
}

public extension MCMapView {
    var camera: MCMapCamera2dInterface {
        mapInterface.getCamera()!
    }

    private func unpackLayer(layer: NSObject?) throws -> MCLayerInterface {
        if let layer = layer as? MCLayerInterface {
            return layer
        }

        // otherwise there might be a "asLayerInterface" method
        let selector = NSSelectorFromString("asLayerInterface")
        guard let layer = layer,
              layer.responds(to: selector) else {
            throw MCMapViewError.invalidLayerType
        }
        guard let wrappedLayer = layer.perform(selector) else {
            throw MCMapViewError.invalidLayerType
        }
        let unretainedLayer = wrappedLayer.takeUnretainedValue()
        guard let result = unretainedLayer as? MCLayerInterface else {
            throw MCMapViewError.invalidLayerType
        }
        return result
    }

    /// layer has to either be a MCLayerInterface or the Object has to respond to the selector "asLayerInterface"
    /// which returns a MCLayerInterface
    func add(layer: NSObject?) throws {
        let layer = try unpackLayer(layer: layer)
        add(layer: layer)
    }
    func add(layer: MCLayerInterface?){
        mapInterface.addLayer(layer)
    }

    func insert(layer: NSObject?, at index: Int) throws {
        let layer = try unpackLayer(layer: layer)
        insert(layer: layer, at: index)
    }
    func insert(layer: MCLayerInterface?, at index: Int){
        mapInterface.insertLayer(at: layer, at: Int32(index))
    }

    func insert(layer: NSObject?, above: NSObject?) throws {
        let layer = try unpackLayer(layer: layer)
        let above = try unpackLayer(layer: above)
        insert(layer: layer, above: above)
    }
    func insert(layer: MCLayerInterface?, above: MCLayerInterface?) {
        mapInterface.insertLayer(above: layer, above: above)
    }

    func insert(layer: NSObject?, below: NSObject?) throws {
        let layer = try unpackLayer(layer: layer)
        let below = try unpackLayer(layer: below)
        mapInterface.insertLayer(below: layer, below: below)
    }
    func insert(layer: MCLayerInterface?, below: MCLayerInterface?) {
        mapInterface.insertLayer(below: layer, below: below)
    }

    func remove(layer: NSObject?) throws {
        let layer = try unpackLayer(layer: layer)
        remove(layer: layer)
    }
    func remove(layer: MCLayerInterface?){
        mapInterface.removeLayer(layer)
    }
}
