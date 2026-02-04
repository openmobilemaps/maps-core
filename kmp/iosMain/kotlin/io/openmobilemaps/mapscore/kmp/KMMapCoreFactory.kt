package io.openmobilemaps.mapscore.kmp

import MapCoreKmp.MapCoreKmpBridge
import platform.Foundation.NSBundle
import platform.Foundation.NSData

object KMMapCoreFactory {
    fun createTextureLoader(): KMLoaderInterface =
        (MapCoreKmpBridge.createTextureLoader() as MapCoreSharedModule.MCLoaderInterfaceProtocol).asKmp()

    fun createFontLoader(bundle: NSBundle, resourcePath: String? = null): KMFontLoaderInterface =
        if (resourcePath == null) {
            (MapCoreKmpBridge.createFontLoaderWithBundle(bundle) as MapCoreSharedModule.MCFontLoaderInterfaceProtocol).asKmp()
        } else {
            (MapCoreKmpBridge.createFontLoaderWithBundle(bundle, resourcePath) as MapCoreSharedModule.MCFontLoaderInterfaceProtocol).asKmp()
        }

    fun createTextureHolder(data: NSData): KMTextureHolderInterface? =
        (MapCoreKmpBridge.createTextureHolderWithData(data) as? MapCoreSharedModule.MCTextureHolderInterfaceProtocol)?.asKmp()
}
