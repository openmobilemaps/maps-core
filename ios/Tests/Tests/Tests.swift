import XCTest
@testable import MapCore

final class Tests: XCTestCase {
    func testExample() {
        XCTAssertEqual(ModuleTemplateSwift().stringFromSwift(), "Hello from Swift")
        XCTAssertEqual(ModuleTemplateSwift().stringFromCpp(), "Hello from the other side")
    }

    static var allTests = [
        ("testExample", testExample),
    ]
}
