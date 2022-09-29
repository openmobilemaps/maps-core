// swift-tools-version:5.5

import PackageDescription

let package = Package(
    name: "LayerGps",
    platforms: [
        .iOS(.v11)
    ],
    products: [
        .library(
            name: "LayerGps",
            targets: ["LayerGps"]
        ),
        .library(
            name: "LayerGpsSharedModule",
            targets: ["LayerGpsSharedModule"]
        ),
        .library(
            name: "LayerGpsSharedModuleCpp",
            targets: ["LayerGpsSharedModuleCpp"]
        ),
    ],
    dependencies: [
        .package(name: "MapCore",
                 url: "https://github.com/openmobilemaps/maps-core.git",
                 .branch("develop")),
        .package(name: "UBKit", url: "https://github.com/UbiqueInnovation/ubkit-ios", .upToNextMajor(from: "1.5.0"))
    ],
    targets: [
        .target(
            name: "LayerGps",
            dependencies: [
                .product(name: "MapCore", package: "MapCore"),
                .product(name: "UBLocation", package: "UBKit"),
                "LayerGpsSharedModule",
            ],
            path: "ios",
            exclude: ["README.md"],
            resources: [
                .process("Resources/Assets.xcassets")
            ]
        ),
        .target(
            name: "LayerGpsSharedModule",
            dependencies: [
                "LayerGpsSharedModuleCpp",
                .product(name: "MapCoreSharedModule", package: "MapCore"),
            ],
            path: "bridging/ios",
            publicHeadersPath: ""
        ),
        .target(
            name: "LayerGpsSharedModuleCpp",
            dependencies: [
                .product(name: "MapCoreSharedModuleCpp", package: "MapCore"),
            ],
            path: "shared",
            sources: ["src"],
            publicHeadersPath: "public",
            cxxSettings: [
                .headerSearchPath("**"),
                .headerSearchPath("public"),
                .headerSearchPath("src/gps"),
            ]
        ),
    ],
    cxxLanguageStandard: .cxx17
)
