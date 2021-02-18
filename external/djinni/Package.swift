// swift-tools-version:5.3
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "DjinniSupport",
    products: [
        .library(
            name: "DjinniSupport",
            targets: ["DjinniSupport"]
        ),
    ],
    targets: [
        .target(
            name: "DjinniSupport",
            path: "support-lib/",
            exclude: ["jni",
                      "java",
                      "ios-build-support-lib.sh",
                      "support_lib.gyp",
                      "support-lib.iml",
                      "proxy_cache_impl.hpp",
                      "proxy_cache_interface.hpp",
                      "djinni_common.hpp"],
            publicHeadersPath: "objc"
        ),
    ],
    cxxLanguageStandard: .cxx1z
)
