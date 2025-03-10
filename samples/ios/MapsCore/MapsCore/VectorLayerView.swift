//
//  ContentView.swift
//  MapsCore
//
//  Created by Nicolas Märki on 08.03.2025.
//

import SwiftUI
import MapCore

struct VectorLayerView: View {
    @State var camera = MapView.Camera(latitude: 47.3769, longitude: 8.5417, zoom: 10_000)
    @State var basemap = VectorLayer(styleURL: "https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json")
    var body: some View {
        MapView(camera: $camera,
                layers: [basemap]
        ).edgesIgnoringSafeArea(.all)
    }
}

#Preview {
    VectorLayerView()
}
