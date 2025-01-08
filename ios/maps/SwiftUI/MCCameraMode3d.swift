import Foundation
//
//  Coord.swift
//  Fluid
//
//  Created by Nicolas Märki on 08.01.2025.
//
import MapCore

let COORDS_ZURICH = MCCoord(lat: 47.37565, lon: 8.54362)

private let GLOBE_MIN_ZOOM: Float = 200_000_000
private let GLOBE_INITIAL_ZOOM: Float = 46_000_000
private let GLOBE_MAX_ZOOM: Float = 5_000_000
private let LOCAL_MIN_ZOOM: Float = 8_000_000
private let LOCAL_INITIAL_ZOOM: Float = 3_000_000
private let LOCAL_MAX_ZOOM: Float = 100_000
private let NO_INTERPOLATION = MCCameraInterpolation(stops: [])
private let FLUID_CAMERA_PITCH = MCCameraInterpolation(
    stops: [
        MCCameraInterpolationValue(stop: 0.0, value: 0.0),
        MCCameraInterpolationValue(
            stop: (GLOBE_MIN_ZOOM - GLOBE_INITIAL_ZOOM)
                / (GLOBE_MIN_ZOOM - GLOBE_MAX_ZOOM),
            value: 25.0
        ),
        MCCameraInterpolationValue(stop: 1.0, value: 0.0),
    ]
)
private let FLUID_VERTICAL_DISPLACEMENT = MCCameraInterpolation(
    stops: [
        MCCameraInterpolationValue(stop: 0.0, value: 0.0),
        MCCameraInterpolationValue(
            stop: (GLOBE_MIN_ZOOM - GLOBE_INITIAL_ZOOM)
                / (GLOBE_MIN_ZOOM - GLOBE_MAX_ZOOM),
            value: 0.3
        ),
        MCCameraInterpolationValue(stop: 1.0, value: 0.0),
    ]
)

private func singleValueMCCameraInterpolation(value: Float)
    -> MCCameraInterpolation
{
    return MCCameraInterpolation(stops: [
        MCCameraInterpolationValue(stop: 0.0, value: value)
    ])
}

public enum MCCameraMode3d: Equatable {
    case ONBOARDING_ROTATING_GLOBE
    case ONBOARDING_ROTATING_SEMI_GLOBE
    case ONBOARDING_CLOSE_ORBITAL
    case ONBOARDING_EVEN_CLOSER_ORBITAL
    case ONBOARDING_FOCUS_ZURICH
    case GLOBAL
    case GLOBE_ROTATING
    case LOCAL

    var cameraConfig: MCCamera3dConfig {
        switch self {
        case .ONBOARDING_ROTATING_GLOBE:
            return MCCamera3dConfig(
                key: "onboardingRotatingGlobe",
                allowUserInteraction: false,
                rotationSpeed: 1.0,
                animationDurationMs: 0,
                minZoom: GLOBE_MIN_ZOOM,
                maxZoom: GLOBE_MAX_ZOOM,
                pitchInterpolationValues: NO_INTERPOLATION,
                verticalDisplacementInterpolationValues:
                    singleValueMCCameraInterpolation(value: 0.1)
            )
        case .ONBOARDING_ROTATING_SEMI_GLOBE:
            return MCCamera3dConfig(
                key: "onboardingRotatingSemiGlobe",
                allowUserInteraction: false,
                rotationSpeed: 1.0,
                animationDurationMs: 2000,
                minZoom: GLOBE_MIN_ZOOM,
                maxZoom: GLOBE_MAX_ZOOM,
                pitchInterpolationValues: NO_INTERPOLATION,
                verticalDisplacementInterpolationValues:
                    singleValueMCCameraInterpolation(value: 0.1)
            )
        case .ONBOARDING_CLOSE_ORBITAL:
            return MCCamera3dConfig(
                key: "onboardingCloseOrbital",
                allowUserInteraction: false,
                rotationSpeed: 1.0,
                animationDurationMs: 2000,
                minZoom: GLOBE_MIN_ZOOM,
                maxZoom: GLOBE_MAX_ZOOM,
                pitchInterpolationValues: FLUID_CAMERA_PITCH,
                verticalDisplacementInterpolationValues:
                    FLUID_VERTICAL_DISPLACEMENT
            )
        case .ONBOARDING_EVEN_CLOSER_ORBITAL:
            return MCCamera3dConfig(
                key: "onboardingEvenCloserOrbital",
                allowUserInteraction: false,
                rotationSpeed: 1.0,
                animationDurationMs: 1000,
                minZoom: GLOBE_MIN_ZOOM,
                maxZoom: GLOBE_MAX_ZOOM,
                pitchInterpolationValues: FLUID_CAMERA_PITCH,
                verticalDisplacementInterpolationValues:
                    FLUID_VERTICAL_DISPLACEMENT
            )
        case .ONBOARDING_FOCUS_ZURICH:
            return MCCamera3dConfig(
                key: "onboardingFocusZurich",
                allowUserInteraction: false,
                rotationSpeed: nil,
                animationDurationMs: 4000,
                minZoom: GLOBE_MIN_ZOOM,
                maxZoom: GLOBE_MAX_ZOOM,
                pitchInterpolationValues: FLUID_CAMERA_PITCH,
                verticalDisplacementInterpolationValues:
                    FLUID_VERTICAL_DISPLACEMENT
            )
        case .GLOBAL:
            return MCCamera3dConfig(
                key: "global",
                allowUserInteraction: true,
                rotationSpeed: nil,
                animationDurationMs: 300,
                minZoom: GLOBE_MIN_ZOOM,
                maxZoom: GLOBE_MAX_ZOOM,
                pitchInterpolationValues: FLUID_CAMERA_PITCH,
                verticalDisplacementInterpolationValues:
                    FLUID_VERTICAL_DISPLACEMENT
            )
        case .GLOBE_ROTATING:
            return MCCamera3dConfig(
                key: "globeRotating",
                allowUserInteraction: false,
                rotationSpeed: 1.0,
                animationDurationMs: 300,
                minZoom: GLOBE_MIN_ZOOM,
                maxZoom: 5_000_000,
                pitchInterpolationValues: FLUID_CAMERA_PITCH,
                verticalDisplacementInterpolationValues:
                    FLUID_VERTICAL_DISPLACEMENT
            )
        case .LOCAL:
            return MCCamera3dConfig(
                key: "local",
                allowUserInteraction: true,
                rotationSpeed: nil,
                animationDurationMs: 300,
                minZoom: LOCAL_MIN_ZOOM,
                maxZoom: LOCAL_MAX_ZOOM,
                pitchInterpolationValues: FLUID_CAMERA_PITCH,
                verticalDisplacementInterpolationValues:
                    FLUID_VERTICAL_DISPLACEMENT
            )
        }
    }

    var targetZoom: Float {
        switch self {
        case .ONBOARDING_ROTATING_GLOBE:
            return GLOBE_MIN_ZOOM
        case .ONBOARDING_ROTATING_SEMI_GLOBE:
            return 100_000_000
        case .ONBOARDING_CLOSE_ORBITAL:
            return 70_000_000
        case .ONBOARDING_EVEN_CLOSER_ORBITAL:
            return 50_000_000
        case .ONBOARDING_FOCUS_ZURICH:
            return 20_000_000
        case .GLOBAL, .GLOBE_ROTATING, .LOCAL:
            return cameraConfig.minZoom
        }
    }

    var targetCoords: MCCoord? {
        switch self {
        case .ONBOARDING_FOCUS_ZURICH:
            return COORDS_ZURICH
        default:
            return nil
        }
    }
}
