import Foundation
import MapCoreSharedModule

extension MCMapConfig {
    static var LV95: MCMapConfig {
        MCMapConfig(mapCoordinateSystem: .init(identifier: "LV95",
                                               boundsLeft: 2_485_071.58,
                                               boundsTop: 1_299_941.79,
                                               boundsRight: 2_828_515.82,
                                               boundsBottom: 1_075_346.31,
                                               zoomMin: 0.0063 / 8.0,
                                               zoomMax: 16))
    }
}
