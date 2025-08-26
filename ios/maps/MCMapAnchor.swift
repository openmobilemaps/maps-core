/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import UIKit
@_exported import MapCoreSharedModule

/// A layout anchor that maintains its position relative to a map coordinate using AutoLayout constraints.
/// The anchor automatically updates its screen position when the map camera changes.
open class MCMapAnchor: NSObject {
    
    // MARK: - Public Properties
    
    /// The map coordinate this anchor is tied to
    public var coordinate: MCCoord {
        didSet {
            updatePosition()
        }
    }
    
    /// AutoLayout anchors for positioning views relative to the map coordinate
    public var centerXAnchor: NSLayoutXAxisAnchor { internalView.centerXAnchor }
    public var centerYAnchor: NSLayoutYAxisAnchor { internalView.centerYAnchor }
    public var leadingAnchor: NSLayoutXAxisAnchor { internalView.leadingAnchor }
    public var trailingAnchor: NSLayoutXAxisAnchor { internalView.trailingAnchor }
    public var topAnchor: NSLayoutYAxisAnchor { internalView.topAnchor }
    public var bottomAnchor: NSLayoutYAxisAnchor { internalView.bottomAnchor }
    
    // MARK: - Private Properties
    
    /// Hidden internal view that participates in the layout system
    private let internalView: UIView
    
    /// Weak reference to the map view to prevent retain cycles
    private weak var mapView: MCMapView?
    
    /// Internal constraints for positioning the anchor view
    private var centerXConstraint: NSLayoutConstraint?
    private var centerYConstraint: NSLayoutConstraint?
    
    // MARK: - Initialization
    
    /// Creates a new map anchor tied to the specified coordinate
    /// - Parameters:
    ///   - coordinate: The map coordinate this anchor should track
    ///   - mapView: The map view this anchor belongs to
    internal init(coordinate: MCCoord, mapView: MCMapView) {
        self.coordinate = coordinate
        self.mapView = mapView
        self.internalView = UIView()
        
        super.init()
        
        setupInternalView()
        updatePosition()
    }
    
    // MARK: - Private Methods
    
    private func setupInternalView() {
        guard let mapView = mapView else { return }
        
        // Configure the internal view
        internalView.isHidden = true
        internalView.isUserInteractionEnabled = false
        internalView.translatesAutoresizingMaskIntoConstraints = false
        
        // Add to map view
        mapView.addSubview(internalView)
        
        // Create initial constraints
        centerXConstraint = internalView.centerXAnchor.constraint(equalTo: mapView.leadingAnchor)
        centerYConstraint = internalView.centerYAnchor.constraint(equalTo: mapView.topAnchor)
        
        centerXConstraint?.isActive = true
        centerYConstraint?.isActive = true
    }
    
    /// Updates the screen position based on the current coordinate and camera state
    internal func updatePosition() {
        guard let mapView = mapView else { return }
        
        // Convert coordinate to screen position using the camera
        let screenPosition = mapView.camera.screenPosFromCoord(coordinate)
        
        // Update constraints to position the internal view at the screen location
        centerXConstraint?.constant = CGFloat(screenPosition.x)
        centerYConstraint?.constant = CGFloat(screenPosition.y)
    }
    
    /// Cleanup method called when the anchor is removed
    internal func cleanup() {
        centerXConstraint?.isActive = false
        centerYConstraint?.isActive = false
        internalView.removeFromSuperview()
        mapView = nil
    }
}