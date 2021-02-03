import Foundation
import MapCoreSharedModule

extension MCMapConfig {
    static var LV95: MCMapConfig {
        MCMapConfig(mapCoordinateSystem: .init(identifier:  "LV95",
                                               boundsLeft:   2420000,
                                               boundsTop:    1350000,
                                               boundsRight:  2900000,
                                               boundsBottom: 1030000,
                                               zoomMin:      14,
                                               zoomMax:      28))
    }
}
