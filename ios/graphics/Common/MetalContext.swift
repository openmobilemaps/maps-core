/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import MetalKit

public class MetalContext {
    static public let current: MetalContext = {
        guard let device = MTLCreateSystemDefaultDevice() else {
            // Metal is available in iOS 13 and tvOS 13 simulators when running on macOS 10.15.
            fatalError("No GPU device found. Are you testing with iOS 12 Simulator?")
        }
        guard let commandQueue = device.makeCommandQueue() else {
            // Metal is available in iOS 13 and tvOS 13 simulators when running on macOS 10.15.
            fatalError("No command queue found. Are you testing with iOS 12 Simulator?")
        }
        do {
            let library = try device.makeDefaultLibrary(bundle: Bundle.module)
            return MetalContext(device: device, commandQueue: commandQueue, library: library)
        } catch {
            fatalError("No Default Metal Library found")
        }
    }()

    public let device: MTLDevice
    let commandQueue: MTLCommandQueue
    public let library: MTLLibrary

    let colorPixelFormat: MTLPixelFormat = .bgra8Unorm
    let textureLoader: MTKTextureLoader

    public lazy var pipelineLibrary: PipelineLibrary = try! PipelineLibrary(device: self.device)
    public lazy var samplerLibrary: SamplerLibrary = try! SamplerLibrary(device: self.device)

    init(device: MTLDevice, commandQueue: MTLCommandQueue, library: MTLLibrary) {
        self.device = device
        self.commandQueue = commandQueue
        self.library = library
        textureLoader = MTKTextureLoader(device: device)
    }
}
