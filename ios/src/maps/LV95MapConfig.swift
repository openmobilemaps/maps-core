import Foundation
import MapCoreSharedModule

extension MCMapConfig {
    static var LV95: MCMapConfig {
        MCMapConfig(mapCoordinateSystem: .init(identifier: "LV95",
                                               boundsLeft: 2485071.58,
                                               boundsTop: 1299941.79,
                                               boundsRight: 2828515.82,
                                               boundsBottom: 1075346.31,
                                               unitToMeterFactor: 1),
                    zoomMin: 40_000_000,
                    zoomMax: 300)
    }
}
