import Foundation
import MapCore
import MapCoreSharedModule

/// Pure swift classes like MCTextureLoader or MCFontLoader can't be created in KMP
/// Bridge helps to expose those APIs.
@objcMembers
public class MapCoreKmpBridge: NSObject {
    @objc(createTextureLoader)
    public class func createTextureLoader() -> MCLoaderInterface {
        return MCTextureLoader()
    }

    @objc(createFontLoaderWithBundle:resourcePath:)
    public class func createFontLoader(bundle: Bundle, resourcePath: String? = nil) -> MCFontLoaderInterface {
        return MCFontLoader(bundle: bundle, resourcePath: resourcePath)
    }

    @objc(createFontLoaderWithBundle:)
    public class func createFontLoader(bundle: Bundle) -> MCFontLoaderInterface {
        return MCFontLoader(bundle: bundle, resourcePath: nil)
    }

    @objc(createTextureHolderWithData:)
    public class func createTextureHolder(data: Data) -> MCTextureHolderInterface? {
        return TextureHolder(data: data)
    }
}
