/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

@_exported import LayerGpsSharedModule
import MapCore
import Foundation
import UIKit

public class MCGpsLayer: NSObject {
    open private(set) var nativeLayer: MCGpsLayerInterface!

    private var callbackHandler = MCGpsCallbackHandler()

    public var modeDidChangeCallback: ((_ mode: MCGpsMode) -> Void)? {
        didSet {
            callbackHandler.modeDidChangeCallback = modeDidChangeCallback
        }
    }

    public init(style: MCGpsStyleInfo = .defaultStyle,
                canAskForPermission: Bool = true,
                nativeLayerProvider: ((MCGpsStyleInfo) -> MCGpsLayerInterface?) = MCGpsLayerInterface.create) {
        nativeLayer = nativeLayerProvider(style)

        super.init()

        nativeLayer.setCallbackHandler(callbackHandler)
    }

    public func setMode(_ mode: MCGpsMode) {
        nativeLayer.setMode(mode)
    }

    public var mode: MCGpsMode {
        nativeLayer.getMode()
    }

    public func asLayerInterface() -> MCLayerInterface? {
        nativeLayer.asLayerInterface()
    }
}

public extension MCGpsStyleInfo {
    static var defaultStyle: MCGpsStyleInfo {
        guard let pointImage = UIImage(named: "ic_gps_point", in: Bundle.module, compatibleWith: nil)!.cgImage,
              let pointTexture = try? TextureHolder(pointImage),
              let headingImage = UIImage(named: "ic_gps_direction", in: Bundle.module, compatibleWith: nil)!.cgImage,
              let headingTexture = try? TextureHolder(headingImage) else {
            fatalError("gps style assets not found")
        }

        return MCGpsStyleInfo(pointTexture: pointTexture,
                              headingTexture: headingTexture,
                              accuracyColor: UIColor(red: 112 / 255,
                                                     green: 173 / 255,
                                                     blue: 204 / 255,
                                                     alpha: 0.2).mapCoreColor)
    }
}

private class MCGpsCallbackHandler: MCGpsLayerCallbackInterface {
    var modeDidChangeCallback: ((_ mode: MCGpsMode) -> Void)?

    func modeDidChange(_ mode: MCGpsMode) {
        DispatchQueue.main.async {
            self.modeDidChangeCallback?(mode)
        }
    }

    func onPointClick(_ coordinate: MCCoord) {
    }
}
