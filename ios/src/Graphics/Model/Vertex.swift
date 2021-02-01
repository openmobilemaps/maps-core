import MetalKit

/// A 2D point in the X-Y coordinate system carrying a texture coordinate
struct Vertex: Equatable {
    /// The 2D position of the vertex in the plane
    var position: SIMD2<Float>
    /// The texture coordinates mapped to the vertex in U-V coordinate space
    var textureCoordinate: SIMD2<Float>
    /// Normal
    var normal: SIMD2<Float>

    /// Returns the descriptor to use when passed to a metal shader
    static let descriptor: MTLVertexDescriptor = {
        let vertexDescriptor = MTLVertexDescriptor()
        var offset = 0
        let bufferIndex = 0
        // Position
        vertexDescriptor.attributes[0].bufferIndex = bufferIndex
        vertexDescriptor.attributes[0].format = .float2
        vertexDescriptor.attributes[0].offset = offset
        offset += MemoryLayout<SIMD2<Float>>.stride
        // UV
        vertexDescriptor.attributes[1].bufferIndex = bufferIndex
        vertexDescriptor.attributes[1].format = .float2
        vertexDescriptor.attributes[1].offset = offset
        offset += MemoryLayout<SIMD2<Float>>.stride
        // Normal
        vertexDescriptor.attributes[2].bufferIndex = bufferIndex
        vertexDescriptor.attributes[2].format = .float2
        vertexDescriptor.attributes[2].offset = offset
        offset += MemoryLayout<SIMD2<Float>>.stride

        vertexDescriptor.layouts[0].stride = MemoryLayout<Vertex>.stride
        return vertexDescriptor
    }()

    /// Initializes a Vertex
    /// - Parameters:
    ///   - x: The X-coordinate position in the plane
    ///   - y: The Y-coordinate position in the plane
    ///   - textureU: The texture U-coordinate mapping
    ///   - textureV: The texture V-coordinate mapping
    init(x: Float, y: Float, textureU: Float, textureV: Float) {
        position = SIMD2([x, y])
        normal = SIMD2([0.0, 0.0])
        textureCoordinate = SIMD2([textureU, textureV])
    }

    init(x: Float, y: Float, normalX: Float, normalY: Float) {
        position = SIMD2([x, y])
        normal = SIMD2([normalX, normalY])
        textureCoordinate = SIMD2([0.0, 0.0])
    }
}

extension Vertex {
    /// The X-coordinate position in the plane
    var x: Float { position.x }
    /// The Y-coordinate position in the plane
    var y: Float { position.y }
    /// The texture U-coordinate mapping
    var textureU: Float { textureCoordinate.x }
    /// The texture V-coordinate mapping
    var textureV: Float { textureCoordinate.x }

    var normalX: Float { normal.x }
    var normalY: Float { normal.y }
}

extension Vertex: CustomDebugStringConvertible {
    var debugDescription: String {
        "<XY: (\(x), \(y)), UV: (\(textureU), \(textureV)), n: (\(normalX), \(normalY))>"
    }
}
