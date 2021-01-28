import MetalKit

class MetalContext {
    static let current: MetalContext = {
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

    let device: MTLDevice
    let commandQueue: MTLCommandQueue
    let library: MTLLibrary

    let colorPixelFormat: MTLPixelFormat = .bgra8Unorm
    let textureLoader: MTKTextureLoader

    lazy var pipelineLibrary: PipelineLibrary = try! PipelineLibrary(device: self.device)
    lazy var samplerLibrary: SamplerLibrary = SamplerLibrary(device: self.device)

    init(device: MTLDevice, commandQueue: MTLCommandQueue, library: MTLLibrary) {
        self.device = device
        self.commandQueue = commandQueue
        self.library = library
        textureLoader = MTKTextureLoader(device: device)
    }
}
