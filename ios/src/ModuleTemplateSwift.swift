import Foundation
import MapCoreSharedModule

public class ModuleTemplateSwift {
    func stringFromSwift() -> String {
        "Hello from Swift"
    }

    func stringFromCpp() -> String {
        if (MapCoreSharedModule.GPHCTestInterface.test(10) == .TEST_1) {
            return "x"
        }
        return ""
    }
}
