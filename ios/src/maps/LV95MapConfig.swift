import Foundation
import MapCoreSharedModule

extension MCMapConfig {
    static var LV95: MCMapConfig {
        MCMapConfig(mapCoordinateSystem: .init(identifier: "LV95",
                                               boundsLeft: 2420000.0,
                                               boundsTop: 1350000.0,
                                               boundsRight: 2900000.0,
                                               boundsBottom: 1030000.0,
                                               unitToMeterFactor: 1),
                    zoomMin: 0,
                    zoomMax: 1)
    }
}
