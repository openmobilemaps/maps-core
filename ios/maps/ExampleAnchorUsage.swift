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

/// Example usage of MCMapAnchor - demonstrates the API as specified in the issue
class ExampleAnchorUsage: UIViewController {
    
    private var mapView: MCMapView!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        setupMapView()
        demonstrateAnchorAPI()
    }
    
    private func setupMapView() {
        // Create map view with basic configuration
        mapView = MCMapView()
        mapView.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(mapView)
        
        NSLayoutConstraint.activate([
            mapView.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor),
            mapView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            mapView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            mapView.bottomAnchor.constraint(equalTo: view.bottomAnchor)
        ])
    }
    
    private func demonstrateAnchorAPI() {
        // Exact example from the issue description:
        
        // Create an anchor for a specific coordinate
        let zurichCoord = MCCoord(lat: 47.3769, lon: 8.5417)
        let anchor = mapView.createAnchor(for: zurichCoord)
        
        // Position any UIView relative to the map coordinate using AutoLayout
        let pinView = createPinView(color: .red)
        NSLayoutConstraint.activate([
            pinView.centerXAnchor.constraint(equalTo: anchor.centerXAnchor),
            pinView.centerYAnchor.constraint(equalTo: anchor.centerYAnchor)
        ])
        
        // The pin automatically stays positioned at the coordinate as users interact with the map
        
        // Additional examples showing different anchor types and dynamic updates:
        demonstrateMultipleAnchors()
        demonstrateDynamicUpdates()
        demonstrateAnchorManagement()
    }
    
    private func demonstrateMultipleAnchors() {
        // Create multiple anchors with different positioning
        let parisCoord = MCCoord(lat: 48.8566, lon: 2.3522)
        let parisAnchor = mapView.createAnchor(for: parisCoord)
        
        let parisPin = createPinView(color: .blue)
        NSLayoutConstraint.activate([
            parisPin.leadingAnchor.constraint(equalTo: parisAnchor.trailingAnchor, constant: 10),
            parisPin.topAnchor.constraint(equalTo: parisAnchor.bottomAnchor, constant: -5)
        ])
        
        // London with different anchor positioning
        let londonCoord = MCCoord(lat: 51.5074, lon: -0.1278)
        let londonAnchor = mapView.createAnchor(for: londonCoord)
        
        let londonLabel = createLabel(text: "London")
        NSLayoutConstraint.activate([
            londonLabel.bottomAnchor.constraint(equalTo: londonAnchor.topAnchor, constant: -5),
            londonLabel.centerXAnchor.constraint(equalTo: londonAnchor.centerXAnchor)
        ])
    }
    
    private func demonstrateDynamicUpdates() {
        // Create an anchor that will move dynamically
        let movingCoord = MCCoord(lat: 46.9481, lon: 7.4474) // Bern
        let movingAnchor = mapView.createAnchor(for: movingCoord)
        
        let movingPin = createPinView(color: .green)
        NSLayoutConstraint.activate([
            movingPin.centerXAnchor.constraint(equalTo: movingAnchor.centerXAnchor),
            movingPin.centerYAnchor.constraint(equalTo: movingAnchor.centerYAnchor)
        ])
        
        // After 2 seconds, move the anchor to a new location
        DispatchQueue.main.asyncAfter(deadline: .now() + 2.0) {
            // Update the coordinate - this should automatically move the pin
            movingAnchor.coordinate = MCCoord(lat: 46.5197, lon: 6.6323) // Geneva
            print("Moved anchor from Bern to Geneva")
        }
    }
    
    private func demonstrateAnchorManagement() {
        // Show anchor lifecycle management
        print("Total anchors before: \(mapView.activeAnchors.count)")
        
        // Create a temporary anchor
        let tempCoord = MCCoord(lat: 47.0502, lon: 8.3093)
        let tempAnchor = mapView.createAnchor(for: tempCoord)
        print("Total anchors after creating temp: \(mapView.activeAnchors.count)")
        
        // Remove it after 3 seconds
        DispatchQueue.main.asyncAfter(deadline: .now() + 3.0) {
            self.mapView.removeAnchor(tempAnchor)
            print("Total anchors after removing temp: \(self.mapView.activeAnchors.count)")
        }
        
        // After 5 seconds, remove all anchors
        DispatchQueue.main.asyncAfter(deadline: .now() + 5.0) {
            print("Removing all anchors...")
            self.mapView.removeAllAnchors()
            print("Total anchors after removing all: \(self.mapView.activeAnchors.count)")
        }
    }
    
    // MARK: - Helper Methods
    
    private func createPinView(color: UIColor) -> UIView {
        let pin = UIView()
        pin.backgroundColor = color
        pin.layer.cornerRadius = 10
        pin.layer.borderWidth = 2
        pin.layer.borderColor = UIColor.white.cgColor
        pin.translatesAutoresizingMaskIntoConstraints = false
        mapView.addSubview(pin)
        
        NSLayoutConstraint.activate([
            pin.widthAnchor.constraint(equalToConstant: 20),
            pin.heightAnchor.constraint(equalToConstant: 20)
        ])
        
        return pin
    }
    
    private func createLabel(text: String) -> UILabel {
        let label = UILabel()
        label.text = text
        label.backgroundColor = UIColor.black.withAlphaComponent(0.7)
        label.textColor = .white
        label.font = UIFont.systemFont(ofSize: 12, weight: .medium)
        label.textAlignment = .center
        label.layer.cornerRadius = 4
        label.clipsToBounds = true
        label.translatesAutoresizingMaskIntoConstraints = false
        mapView.addSubview(label)
        
        NSLayoutConstraint.activate([
            label.heightAnchor.constraint(equalToConstant: 24),
            label.widthAnchor.constraint(greaterThanOrEqualToConstant: 60)
        ])
        
        return label
    }
}