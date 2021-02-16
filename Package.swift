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
        .library(
            name: "MapCoreSharedModuleCpp",
            targets: ["MapCoreSharedModuleCpp"]
        ),
    ],
    dependencies: [
        .package(name: "DjinniSupport",
                 url: "git@bitbucket.org:ubique-innovation/djinni.git",
                 .branch("master")),
    ],
    targets: [
        .target(
            name: "MapCore",
            dependencies: ["MapCoreSharedModule"],
            path: "ios",
            resources: [
                .process("ios/graphics/Shader/Metal"),
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
            publicHeadersPath: "public/**",
            cxxSettings: [
                .headerSearchPath("**"),
                .unsafeFlags(["-fno-sized-deallocation",
                              "-fno-aligned-allocation"]),
            ]
        ),
    ],
    cxxLanguageStandard: .cxx1z
)
