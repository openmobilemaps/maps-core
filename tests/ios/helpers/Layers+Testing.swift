//
//  Layers+Testing.swift
//  MapCore
//
//  Created by Nicolas Märki on 21.02.2025.
//

import MapCore

extension VectorLayer {
    convenience init(testingStyleURL: String, loader: MCTextureLoader) {
        self.init(styleURL: testingStyleURL, loaders: [loader], fontLoader: MCFontLoader(bundle: Bundle.module))
    }
}
