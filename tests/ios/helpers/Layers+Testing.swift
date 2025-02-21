//
//  Layers+Testing.swift
//  MapCore
//
//  Created by Nicolas Märki on 21.02.2025.
//

import MapCore

extension VectorLayer {
    convenience init(testingStyleURL: String) {
        self.init(styleURL: "https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json", loaders: [StubTextureLoader()], fontLoader: MCFontLoader(bundle: Bundle.module))
    }
}
