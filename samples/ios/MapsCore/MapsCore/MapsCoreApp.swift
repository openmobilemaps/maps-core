//
//  MapsCoreApp.swift
//  MapsCore
//
//  Created by Nicolas Märki on 08.03.2025.
//

import SwiftUI

@main
struct MapsCoreApp: App {
    var body: some Scene {
        WindowGroup {
            NavigationStack {
                List {
                    NavigationLink("VectorLayer") {
                        VectorLayerView()
                    }
                }
            }
        }
    }
}
