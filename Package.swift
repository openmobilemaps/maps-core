// swift-tools-version:5.3

import PackageDescription

let package = Package(
    name: "MapCore",
    platforms: [
        .iOS(.v11),
    ],
    products: [
        .library(
            name: "MapCore",
            targets: ["MapCore"]
        ),
        .library(
            name: "MapCoreSharedModule",
            targets: ["MapCoreSharedModule"]
        ),
        .library(
            name: "MapCoreSharedModuleCpp",
            targets: ["MapCoreSharedModuleCpp"]
        ),
    ],
    dependencies: [
        .package(name: "DjinniSupport",
                 url: "https://github.com/UbiqueInnovation/djinni.git",
                 .upToNextMajor(from: "1.0.0")),
    ],
    targets: [
        .target(
            name: "MapCore",
            dependencies: ["MapCoreSharedModule"],
            path: "ios",
            exclude: ["readme.md"],
            resources: [
                .process("ios/graphics/Shader/Metal/"),
            ]
        ),
        .target(
            name: "MapCoreSharedModule",
            dependencies: ["DjinniSupport",
                           "MapCoreSharedModuleCpp"],
            path: "bridging/ios",
            publicHeadersPath: ""
        ),
        .target(
            name: "MapCoreSharedModuleCpp",
            path: "shared",
            sources: ["src"],
            publicHeadersPath: "public",
            cxxSettings: [
                .headerSearchPath("**"),
                .headerSearchPath("public"),
                .headerSearchPath("src/graphics"),
                .headerSearchPath("src/graphics/helpers"),
                .headerSearchPath("src/logger"),
                .headerSearchPath("src/map/camera"),
                .headerSearchPath("src/map/controls"),
                .headerSearchPath("src/map/coordinates"),
                .headerSearchPath("src/map/layers/objects"),
                .headerSearchPath("src/map/layers/icon"),
                .headerSearchPath("src/map/layers/line"),
                .headerSearchPath("src/map/layers/tiled"),
                .headerSearchPath("src/map/layers/tiled/raster"),
                .headerSearchPath("src/map/layers/tiled/wmts"),
                .headerSearchPath("src/map/scheduling"),
                .headerSearchPath("src/map"),
                .headerSearchPath("src/external/pugixml"),

            ]
        ),
    ],
    cxxLanguageStandard: .cxx1z
)
