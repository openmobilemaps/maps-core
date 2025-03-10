import Foundation

// Root style definition
struct VectorStyle: Codable {
    var version: Int
    var name: String?
//    var metadata: [String: String]?
    var sources: [String: Source]
    var layers: [Layer]
    var sprite: String?
    var glyphs: String?
    var transition: Transition?
    var bearing: Double?
    var pitch: Double?
    var center: [Double]?
    var zoom: Double?
}

// Source definition
struct Source: Codable {
    let type: String
    let url: String?
    let tiles: [String]?
    let minzoom: Int?
    let maxzoom: Int?
}

// Layer definition
struct Layer: Codable {
    let id: String
    let type: String
    let source: String?
    let sourceLayer: String?
    let minzoom: Int?
    let maxzoom: Int?
    let layout: [String: JSONValue]?
    let paint: [String: JSONValue]?
    let filter: [JSONValue]?

    enum CodingKeys: String, CodingKey {
        case id
        case type
        case source
        case sourceLayer = "source-layer"
        case minzoom
        case maxzoom
        case layout
        case paint
        case filter
    }
}

// Transition settings
struct Transition: Codable {
    let duration: Double?
    let delay: Double?
}

// Custom JSONValue enum to handle dynamic JSON values
enum JSONValue: Codable {
    case string(String)
    case number(Double)
    case bool(Bool)
    case array([JSONValue])
    case dictionary([String: JSONValue])

    init(from decoder: Decoder) throws {
        let container = try decoder.singleValueContainer()
        if let value = try? container.decode(String.self) {
            self = .string(value)
        } else if let value = try? container.decode(Double.self) {
            self = .number(value)
        } else if let value = try? container.decode(Bool.self) {
            self = .bool(value)
        } else if let value = try? container.decode([JSONValue].self) {
            self = .array(value)
        } else if let value = try? container.decode([String: JSONValue].self) {
            self = .dictionary(value)
        } else {
            throw DecodingError.dataCorruptedError(in: container, debugDescription: "Invalid JSON Value")
        }
    }

    func encode(to encoder: Encoder) throws {
        var container = encoder.singleValueContainer()
        switch self {
            case .string(let value):
                try container.encode(value)
            case .number(let value):
                try container.encode(value)
            case .bool(let value):
                try container.encode(value)
            case .array(let value):
                try container.encode(value)
            case .dictionary(let value):
                try container.encode(value)
        }
    }
}
