// swift-tools-version:5.3

import PackageDescription

let package = Package(
    name: "MapCore",
    platforms: [
        .iOS(.v10),
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
    ],
    dependencies: [
        .package(name: "DjinniSupport", url: "git@bitbucket.org:ubique-innovation/djinni.git", .branch("master")),
    ],
    targets: [
        .target(
            name: "MapCore",
            dependencies: ["MapCoreSharedModule"],
            path: "ios/src",
            resources: [
                .process("ios/src/graphics/Shader/Metal"),
            ]
        ),
        .target(
            name: "MapCoreSharedModule",
            dependencies: ["DjinniSupport"],
            path: "shared",
            exclude: ["generated/jni",
                      "generated/java",
                      "generated/graphics",
                      "generated/map"],
            publicHeadersPath: "generated/objc/objc",
            cxxSettings: [
                .headerSearchPath("**"),
                .headerSearchPath("src/graphics/"),
                .headerSearchPath("src/graphics/common"),
                .headerSearchPath("src/graphics/helpers"),
                .headerSearchPath("src/graphics/objects"),
                .headerSearchPath("src/graphics/shader"),
                .headerSearchPath("src/map/"),
                .headerSearchPath("src/map/controls"),
                .headerSearchPath("src/map/layers"),
                .headerSearchPath("src/map/layers/objects"),
                .headerSearchPath("src/map/loader"),
                .headerSearchPath("src/map/scheduling"),
                .headerSearchPath("src/logger"),
            ]
        ),
        .testTarget(
            name: "MapCoreTests",
            dependencies: ["MapCore"],
            path: "ios/Tests/Tests"
        ),
    ],
    cxxLanguageStandard: .cxx1z
)
