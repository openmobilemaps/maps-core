import UIKit

extension UIScreen {
    static let pixelsPerInch: CGFloat = {
        switch UIDevice.modelIdentifier {
        case "iPad2,1", "iPad2,2", "iPad2,3", "iPad2,4":             // iPad 2
            return 132

        case "iPad2,5", "iPad2,6", "iPad2,7":                        // iPad Mini
            return 163

        case "iPad3,1", "iPad3,2", "iPad3,3":            fallthrough // iPad 3rd generation
        case "iPad3,4", "iPad3,5", "iPad3,6":            fallthrough // iPad 4th generation
        case "iPad4,1", "iPad4,2", "iPad4,3":            fallthrough // iPad Air
        case "iPad5,3", "iPad5,4":                       fallthrough // iPad Air 2
        case "iPad6,7", "iPad6,8":                       fallthrough // iPad Pro (12.9 inch)
        case "iPad6,3", "iPad6,4":                       fallthrough // iPad Pro (9.7 inch)
        case "iPad6,11", "iPad6,12":                     fallthrough // iPad 5th generation
        case "iPad7,1", "iPad7,2":                       fallthrough // iPad Pro (12.9 inch, 2nd generation)
        case "iPad7,3", "iPad7,4":                       fallthrough // iPad Pro (10.5 inch)
        case "iPad7,5", "iPad7,6":                       fallthrough // iPad 6th generation
        case "iPad7,11", "iPad7,12":                     fallthrough // iPad 7th generation
        case "iPad8,1", "iPad8,2", "iPad8,3", "iPad8,4": fallthrough // iPad Pro (11 inch)
        case "iPad8,5", "iPad8,6", "iPad8,7", "iPad8,8": fallthrough // iPad Pro (12.9 inch, 3rd generation)
        case "iPad8,9", "iPad8,10":                      fallthrough // iPad Pro (11 inch, 2nd generation)
        case "iPad8,11", "iPad8,12":                     fallthrough // iPad Pro (12.9 inch, 4th generation)
        case "iPad11,3", "iPad11,4":                                 // iPad Air (3rd generation)
            return 264

        case "iPhone4,1":                                fallthrough // iPhone 4S
        case "iPhone5,1", "iPhone5,2":                   fallthrough // iPhone 5
        case "iPhone5,3", "iPhone5,4":                   fallthrough // iPhone 5C
        case "iPhone6,1", "iPhone6,2":                   fallthrough // iPhone 5S
        case "iPhone8,4":                                fallthrough // iPhone SE
        case "iPhone7,2":                                fallthrough // iPhone 6
        case "iPhone8,1":                                fallthrough // iPhone 6S
        case "iPhone9,1", "iPhone9,3":                   fallthrough // iPhone 7
        case "iPhone10,1", "iPhone10,4":                 fallthrough // iPhone 8
        case "iPhone11,8":                               fallthrough // iPhone XR
        case "iPhone12,1":                               fallthrough // iPhone 11
        case "iPhone12,8":                               fallthrough // iPhone SE 2nd generation
        case "iPod5,1":                                  fallthrough // iPod Touch 5th generation
        case "iPod7,1":                                  fallthrough // iPod Touch 6th generation
        case "iPod9,1":                                  fallthrough // iPod Touch 7th generation
        case "iPad4,4", "iPad4,5", "iPad4,6":            fallthrough // iPad Mini 2
        case "iPad4,7", "iPad4,8", "iPad4,9":            fallthrough // iPad Mini 3
        case "iPad5,1", "iPad5,2":                       fallthrough // iPad Mini 4
        case "iPad11,1", "iPad11,2":                                 // iPad Mini 5
            return 326

        case "iPhone7,1":                                fallthrough // iPhone 6 Plus
        case "iPhone8,2":                                fallthrough // iPhone 6S Plus
        case "iPhone9,2", "iPhone9,4":                   fallthrough // iPhone 7 Plus
        case "iPhone10,2", "iPhone10,5":                             // iPhone 8 Plus
            return 401

        case "iPhone10,3", "iPhone10,6":                 fallthrough // iPhone X
        case "iPhone11,2":                               fallthrough // iPhone XS
        case "iPhone12,3":                               fallthrough // iPhone 11 Pro
        case "iPhone11,4", "iPhone11,6":                 fallthrough // iPhone XS Max
        case "iPhone12,5":                                           // iPhone 11 Pro Max
            return 458

        default:                                                     // unknown model identifier
            return 458
        }
    }()
}


private extension UIDevice {

    // model identifiers can be found at https://www.theiphonewiki.com/wiki/Models
    static let modelIdentifier: String = {
        if let simulatorModelIdentifier = ProcessInfo().environment["SIMULATOR_MODEL_IDENTIFIER"] { return simulatorModelIdentifier }
        var sysinfo = utsname()
        uname(&sysinfo) // ignore return value
        return String(bytes: Data(bytes: &sysinfo.machine, count: Int(_SYS_NAMELEN)), encoding: .ascii)!.trimmingCharacters(in: .controlCharacters)
    }()

}
