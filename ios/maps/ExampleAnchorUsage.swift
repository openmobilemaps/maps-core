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
import MapCore

/// Example usage of MCMapAnchor
class ExampleAnchorUsage: UIViewController {
    
    private var mapView: MCMapView!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // Create map view
        mapView = MCMapView()
        mapView.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(mapView)
        
        NSLayoutConstraint.activate([
            mapView.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor),
            mapView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            mapView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            mapView.bottomAnchor.constraint(equalTo: view.bottomAnchor)
        ])
        
        // Example usage as described in the issue
        setupAnchorExample()
    }
    
    private func setupAnchorExample() {
        // Create an anchor for a specific coordinate (Zurich)
        let zurichCoord = MCCoord(lat: 47.3769, lon: 8.5417)
        let anchor = mapView.createAnchor(for: zurichCoord)
        
        // Position any UIView relative to the map coordinate using AutoLayout
        let pinView = UIView()
        pinView.backgroundColor = .red
        pinView.layer.cornerRadius = 10
        pinView.translatesAutoresizingMaskIntoConstraints = false
        mapView.addSubview(pinView)
        
        NSLayoutConstraint.activate([
            pinView.centerXAnchor.constraint(equalTo: anchor.centerXAnchor),
            pinView.centerYAnchor.constraint(equalTo: anchor.centerYAnchor),
            pinView.widthAnchor.constraint(equalToConstant: 20),
            pinView.heightAnchor.constraint(equalToConstant: 20)
        ])
        
        // The pin automatically stays positioned at the coordinate as users interact with the map
        
        // Example: Create multiple anchors
        let parisCoord = MCCoord(lat: 48.8566, lon: 2.3522)
        let parisAnchor = mapView.createAnchor(for: parisCoord)
        
        let parisPin = UIView()
        parisPin.backgroundColor = .blue
        parisPin.layer.cornerRadius = 8
        parisPin.translatesAutoresizingMaskIntoConstraints = false
        mapView.addSubview(parisPin)
        
        NSLayoutConstraint.activate([
            parisPin.centerXAnchor.constraint(equalTo: parisAnchor.centerXAnchor),
            parisPin.centerYAnchor.constraint(equalTo: parisAnchor.centerYAnchor),
            parisPin.widthAnchor.constraint(equalToConstant: 16),
            parisPin.heightAnchor.constraint(equalToConstant: 16)
        ])
        
        // Example: Update coordinate dynamically
        DispatchQueue.main.asyncAfter(deadline: .now() + 3.0) {
            // Move the anchor to a new location
            anchor.coordinate = MCCoord(lat: 47.4058, lon: 8.5498) // Different location in Zurich
        }
        
        print("Created \(mapView.activeAnchors.count) anchors")
    }
}