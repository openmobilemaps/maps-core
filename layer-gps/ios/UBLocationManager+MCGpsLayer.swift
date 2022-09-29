import CoreLocation
import UIKit
import UBLocation

private var binder: LocationManagerLayerBinder?

public extension UBLocationManager {
    func bindTo(layer: MCGpsLayer, canAskForPermission: Bool = false) {
        binder = LocationManagerLayerBinder(layer: layer)

        guard let binder = binder else {
            return
        }

        startLocationMonitoring(for: [.location(background: false), .heading(background: false)],
                                delegate: binder,
                                canAskForPermission: canAskForPermission)

        layer.setMode(.STANDARD)
    }
}

class LocationManagerLayerBinder: NSObject {
    weak var layer: MCGpsLayer?
    public init(layer: MCGpsLayer) {
        self.layer = layer
        super.init()
    }
}

extension LocationManagerLayerBinder: UBLocationManagerDelegate {
    public func locationManager(_ manager: UBLocationManager, requiresPermission permission: UBLocationManager.AuthorizationLevel) {
    }

    public var locationManagerFilterAccuracy: CLLocationAccuracy? { nil }

    public func locationManager(_: UBLocationManager, grantedPermission _: UBLocationManager.AuthorizationLevel, accuracy: UBLocationManager.AccuracyLevel) {
    }

    public func locationManager(permissionDeniedFor manager: UBLocationManager) {
        layer?.setMode(.DISABLED)
    }

    public func locationManager(_ manager: UBLocationManager, didUpdateLocations locations: [CLLocation]) {
        guard let location = locations.last else { return }

        layer?.nativeLayer.setDrawPoint(manager.accuracyLevel == .full)
        layer?.nativeLayer.setDrawHeading(manager.accuracyLevel == .full)

        layer?.nativeLayer.updatePosition(.init(systemIdentifier: MCCoordinateSystemIdentifiers.epsg4326(),
                                                x: location.coordinate.longitude,
                                                y: location.coordinate.latitude,
                                                z: location.altitude), horizontalAccuracyM: location.horizontalAccuracy)
    }

    public func locationManager(_: UBLocationManager, didFailWithError _: Error) {
        layer?.setMode(.DISABLED)
    }

    public func locationManager(_: UBLocationManager, didUpdateHeading newHeading: CLHeading) {
        var h = Float(newHeading.trueHeading)

        switch UIDevice.current.orientation {
        case .landscapeRight:
            h += 90
        case .landscapeLeft:
            h -= 90
        default:
            break
        }

        layer?.nativeLayer.updateHeading(h)
    }
}
