// swift-tools-version:5.2

import PackageDescription

let package = Package(
    name: "MapCore",
    platforms: [
        .iOS(.v10)
    ],
    products: [
        .library(
            name: "MapCore",
            targets: ["MapCore"]),
        .library(
            name: "MapCoreSharedModule",
            targets: ["MapCoreSharedModule"])
    ],
    dependencies: [
        .package(name: "DjinniSupport", url: "https://bitbucket.org/ubique-innovation/djinni", .branch("master"))
    ],
    targets: [
        .target(
            name: "MapCore",
            dependencies: ["MapCoreSharedModule"],
            path: "ios/src"),
        .target(
            name: "MapCoreSharedModule",
            dependencies: ["DjinniSupport"],
            path: "shared",
            exclude: ["generated/jni",
                      "generated/java"],
            publicHeadersPath: "generated/objc/objc",
            cxxSettings: [
            .headerSearchPath("**")
            ]),
        .testTarget(
            name: "Tests",
            dependencies: ["MapCore"],
            path: "ios/Tests/Tests"),
    ],
    cxxLanguageStandard: .cxx1z
)
